#include "FDSRegistration.h"
#include "FDSBeacon.h"
#include "FDSMainStage.h"
#include "FDSVenues.h"
#include "FuserServer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    constexpr int32 NetVersion = 10402; // FUSER EGS v1.4.2 (FuserServer.cpp)
    constexpr double RetrySeconds = 5.0;

    FString Quote(const FString& Value)
    {
        FString Out = Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
        return TEXT("\"") + Out + TEXT("\"");
    }

    // The version the distribution build wrote into its MANIFEST.json (the image's /opt/fds), else "dev".
    FString ManifestVersion()
    {
        FString Text;
        TSharedPtr<FJsonObject> Manifest;
        if (FFileHelper::LoadFileToString(Text, *(FPaths::RootDir() / TEXT("MANIFEST.json"))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) && Manifest.IsValid())
        {
            FString Version;
            if (Manifest->TryGetStringField(TEXT("version"), Version))
            {
                return Version;
            }
        }
        return TEXT("dev");
    }

    // The UDP port the game net driver is bound to: its local address ("ip:port"), which can differ from the URL's port
    // when the net driver moved past a taken one. 0 when the world is not listening.
    int32 BoundGamePort(UWorld* World)
    {
        UNetDriver* Driver = World ? World->GetNetDriver() : nullptr;
        if (!Driver)
        {
            return 0;
        }
        FString Address = Driver->LowLevelGetNetworkNumber(), Host, Port;
        if (Address.Split(TEXT(":"), &Host, &Port, ESearchCase::CaseSensitive, ESearchDir::FromEnd) && FCString::Atoi(*Port) > 0)
        {
            return FCString::Atoi(*Port);
        }
        return World->URL.Port;
    }
}

FFDSRegistration::FFDSRegistration()
{
    Url = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_REGISTER_URL")).TrimStartAndEnd();
    SlotId = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_SLOT_ID")).TrimStartAndEnd();
    Token = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_REGISTER_TOKEN")).TrimStartAndEnd();
    const int32 Heartbeat = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_HEARTBEAT_SECONDS")));
    HeartbeatSeconds = Heartbeat > 0 ? Heartbeat : 10.0f;
    bExitWhenEmpty = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_EXIT_WHEN_EMPTY")) != TEXT("0") && !FDSMainStage::IsEnabled();
    bConfiguredExitWhenEmpty = bExitWhenEmpty;
    while (Url.EndsWith(TEXT("/")))
    {
        Url = Url.LeftChop(1);
    }
    bEnabled = !Url.IsEmpty() && !SlotId.IsEmpty() && !Token.IsEmpty();
    Version = ManifestVersion();
    UE_LOG(LogFDS, Display, TEXT("FDS: fuserd registration %s (version %s%s)"),
        bEnabled ? *FString::Printf(TEXT("on: slot %s at %s"), *SlotId, *Url) : TEXT("off (no FDS_REGISTER_URL / FDS_SLOT_ID / FDS_REGISTER_TOKEN)"),
        *Version, bEnabled ? (bExitWhenEmpty ? TEXT(", exits when its last player leaves") : TEXT(", stays up when empty")) : TEXT(""));
}

FFDSRegistration::~FFDSRegistration() = default;

FFDSRegistration& FFDSRegistration::Get()
{
    static FFDSRegistration Instance; // process lifetime: HTTP callbacks may land after the world is gone
    return Instance;
}

void FFDSRegistration::Start(UWorld* InWorld)
{
    World = InWorld;
    bStarted = bEnabled;
    NextSendAt = 0.0; // register at the next tick
    LastReplyAt = FPlatformTime::Seconds();
}

// port / beacon_port are the ports actually bound (0: not listening): fuserd stops a container whose ports are not the
// slot's at once, instead of waiting out its startup timeout. Both are always sent, so a 0 counts as a mismatch too.
FString FFDSRegistration::RegistrationBody() const
{
    const int32 Port = BoundGamePort(World.Get());
    const int32 BeaconPort = FDSBeacon::BoundPort();
    if (World.IsValid() && (Port == 0 || BeaconPort == 0))
    {
        // An explicit 0 is a port mismatch to fuserd: it stops this server and keeps the slot's ports on cooldown.
        UE_LOG(LogFDS, Error, TEXT("FDS: not listening on %s (UDP %d / beacon %d, assigned %d / %d): fuserd will stop this server"),
            Port == 0 ? TEXT("the game port") : TEXT("the beacon port"), Port, BeaconPort, World->URL.Port, World->URL.Port + 100);
    }
    else if (World.IsValid() && (Port != World->URL.Port || BeaconPort != World->URL.Port + 100))
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: bound UDP %d / beacon %d, not the assigned %d / %d: fuserd will stop this server"), Port, BeaconPort,
            World->URL.Port, World->URL.Port + 100);
    }
    const FDSVenues::FSelection& Venue = FDSVenues::Selected();
    return FString::Printf(TEXT("{\"slot_id\":%s,\"version\":%s,\"net_version\":%d,\"port\":%d,\"beacon_port\":%d,\"venue\":%s,\"map\":%s,")
        TEXT("\"time_of_day\":%s,\"rounds\":%d,\"turn_measures\":%d}"),
        *Quote(SlotId), *Quote(Version), NetVersion, Port, BeaconPort, *Quote(Venue.Id), *Quote(Venue.Map), *Quote(Venue.TimeOfDay),
        Last.Rounds, Last.TurnMeasures);
}

// Members are hype user ids with their team (dj / spectator / unknown), in join order; current_dj is the turn's DJ
// (0 = none), event the Main Stage event being played ("" = none) and recycle_pending whether the server is past
// FDS_MAX_UPTIME_HOURS, waiting to be empty and in the Lobby to exit.
FString FFDSRegistration::StatusBody(const FFDSLobbyStatus& Status) const
{
    FString Members;
    for (const TPair<int32, EMultiplayerTeam>& Member : Status.Members)
    {
        const TCHAR* Team = Member.Value == EMultiplayerTeam::DJ ? TEXT("dj") : Member.Value == EMultiplayerTeam::Spectator ? TEXT("spectator") : TEXT("unknown");
        Members += FString::Printf(TEXT("%s{\"user_id\":%d,\"team\":\"%s\"}"), Members.IsEmpty() ? TEXT("") : TEXT(","), Member.Key, Team);
    }
    return FString::Printf(TEXT("{\"slot_id\":%s,\"players\":%d,\"djs\":%d,\"viewers\":%d,\"stage\":%d,\"round\":%d,\"members\":[%s],")
        TEXT("\"current_dj\":%d,\"event\":%s,\"recycle_pending\":%s}"), *Quote(SlotId), Status.Players, Status.DJs, Status.Viewers, Status.Stage,
        Status.Round, *Members, Status.CurrentDJ, *Quote(Status.Event), bRecyclePending ? TEXT("true") : TEXT("false"));
}

void FFDSRegistration::Send(const TCHAR* Call, const FString& Body)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("%s/fds/v1/%s"), *Url, Call));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + Token);
    Request->SetContentAsString(Body);
    Request->OnProcessRequestComplete().BindRaw(this, &FFDSRegistration::OnReply, FString(Call));
    bInFlight = true;
    Request->ProcessRequest();
}

void FFDSRegistration::Post(const TCHAR* Call, const FString& Body, TFunction<void(int32, const FString&)> Done)
{
    if (!bEnabled)
    {
        Done(0, FString());
        return;
    }
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("%s/fds/v1/%s"), *Url, Call));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + Token);
    Request->SetContentAsString(Body);
    Request->OnProcessRequestComplete().BindLambda([Done](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
    {
        const bool bReplied = bOk && Response.IsValid();
        Done(bReplied ? Response->GetResponseCode() : 0, bReplied ? Response->GetContentAsString() : FString());
    });
    Request->ProcessRequest();
}

// Cels are named like the official ones: <short> + bt|bs|ld|lp (e.g. alotbt), optionally "_" before the part and
// "_trans" after it (FuserProject actions.lua customSongOf). Matching the whole name keeps a short custom name from
// catching an official cel.
bool FFDSRegistration::IsCustomSongBlocked(FName CelShortName, FString* OutSong) const
{
    if (BlockedSongs.Num() == 0 || CelShortName.IsNone())
    {
        return false;
    }
    const FString Cel = CelShortName.ToString().ToLower();
    for (const FString& Song : BlockedSongs)
    {
        if (!Cel.StartsWith(Song, ESearchCase::CaseSensitive))
        {
            continue;
        }
        FString Part = Cel.Mid(Song.Len());
        Part.RemoveFromStart(TEXT("_"), ESearchCase::CaseSensitive);
        Part.RemoveFromEnd(TEXT("_trans"), ESearchCase::CaseSensitive);
        if (Part == TEXT("bt") || Part == TEXT("bs") || Part == TEXT("ld") || Part == TEXT("lp"))
        {
            if (OutSong)
            {
                *OutSong = Song;
            }
            return true;
        }
    }
    return false;
}

void FFDSRegistration::OnReply(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk, FString Call)
{
    bInFlight = false;
    const int32 Code = bOk && Response.IsValid() ? Response->GetResponseCode() : 0;
    // Orphan detection (Tick): the run of 401s continues through calls that got no reply.
    if (Code != 0)
    {
        LastReplyAt = FPlatformTime::Seconds();
        if (Code != 401)
        {
            FirstUnauthorizedAt = -1.0;
        }
        else if (FirstUnauthorizedAt < 0.0)
        {
            FirstUnauthorizedAt = LastReplyAt;
        }
    }
    if (Code == 200 && bOrphaned)
    {
        bOrphaned = false;
        bExitWhenEmpty = bConfiguredExitWhenEmpty;
        UE_LOG(LogFDS, Display, TEXT("FDS: fuserd accepts this server again: no longer exiting when empty unless configured to"));
    }
    if (Code != 200)
    {
        // Logged without the request: its header carries the token.
        UE_LOG(LogFDS, Warning, TEXT("FDS: fuserd %s failed (%s); retrying in %.0f s"), *Call,
            Code ? *FString::Printf(TEXT("HTTP %d"), Code) : TEXT("no reply"), RetrySeconds);
        NextSendAt = FPlatformTime::Seconds() + RetrySeconds;
        return;
    }
    TSharedPtr<FJsonObject> Reply;
    if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Reply) && Reply.IsValid())
    {
        int32 Heartbeat = 0;
        if (Reply->TryGetNumberField(TEXT("heartbeat_seconds"), Heartbeat) && Heartbeat > 0)
        {
            HeartbeatSeconds = Heartbeat;
        }
        const TSharedPtr<FJsonObject>* MainStage = nullptr;
        if (Reply->TryGetObjectField(TEXT("main_stage"), MainStage) && MainStage)
        {
            FDSMainStage::ReceiveSchedule(*MainStage); // the Main Stage server's schedule
        }
        FString Session;
        if (Reply->TryGetStringField(TEXT("session_id"), Session) && !Session.IsEmpty() && Session != SessionIdValue)
        {
            SessionIdValue = Session;
            UE_LOG(LogFDS, Display, TEXT("FDS: fuserd game session %s"), *SessionIdValue);
        }
        // The custom songs not every player in the session can load (fuserd's whole list, every reply; an older
        // fuserd sends none and the list stays empty). The lobby drops SpawnDiscs of these (bBlockUnreadyCustomSongs).
        const TArray<TSharedPtr<FJsonValue>>* Blocked = nullptr;
        if (Reply->TryGetArrayField(TEXT("blocked_songs"), Blocked) && Blocked)
        {
            TArray<FString> Next;
            for (const TSharedPtr<FJsonValue>& Value : *Blocked)
            {
                FString Song;
                if (Value.IsValid() && Value->TryGetString(Song))
                {
                    Song = Song.TrimStartAndEnd().ToLower();
                    if (!Song.IsEmpty())
                    {
                        Next.AddUnique(Song);
                    }
                }
            }
            // Longest first: a shorter shortname that prefixes a longer one must not claim its cels.
            Next.Sort([](const FString& A, const FString& B) { return A.Len() != B.Len() ? A.Len() > B.Len() : A < B; });
            if (Next != BlockedSongs)
            {
                BlockedSongs = MoveTemp(Next);
                UE_LOG(LogFDS, Display, TEXT("FDS: fuserd blocks %d custom song(s) not loaded by every player%s%s"), BlockedSongs.Num(),
                    BlockedSongs.Num() ? TEXT(": ") : TEXT(""), *FString::Join(BlockedSongs, TEXT(", ")));
            }
        }
    }
    if (Call == TEXT("register") && !bRegistered)
    {
        bRegistered = true;
        UE_LOG(LogFDS, Display, TEXT("FDS: registered with fuserd as slot %s (heartbeat %.0f s)"), *SlotId, HeartbeatSeconds);
    }
    NextSendAt = FPlatformTime::Seconds() + HeartbeatSeconds;
}

void FFDSRegistration::Tick(float DeltaSeconds, const FFDSLobbyStatus& Status)
{
    if (!bStarted || bExitSent)
    {
        return;
    }
    const bool bChanged = Status.Players != Last.Players || Status.DJs != Last.DJs || Status.Stage != Last.Stage || Status.Round != Last.Round ||
        !Status.SameLive(Last);
    Last = Status;
    if (Status.Players > 0)
    {
        bHadPlayers = true;
    }
    const double Now = FPlatformTime::Seconds();
    // A container fuserd does not know (an earlier fuserd run started it; fuserd answers its token with 401, or is
    // gone): it would otherwise retry forever. An idle server is not one: fuserd keeps ready servers waiting.
    if (!bOrphaned && ((FirstUnauthorizedAt >= 0.0 && Now - FirstUnauthorizedAt >= UnauthorizedExitSeconds) || Now - LastReplyAt >= NoReplyExitSeconds))
    {
        bOrphaned = true;
        const TCHAR* Why = FirstUnauthorizedAt >= 0.0 ? TEXT("fuserd refuses this server's token") : TEXT("no reply from fuserd");
        if (Status.Players == 0)
        {
            UE_LOG(LogFDS, Warning, TEXT("FDS: %s for %.0f s and no player is connected: exiting"), Why,
                FirstUnauthorizedAt >= 0.0 ? Now - FirstUnauthorizedAt : Now - LastReplyAt);
            bExitSent = true; // nobody to report the exit to
            FPlatformMisc::RequestExit(false);
            return;
        }
        UE_LOG(LogFDS, Warning, TEXT("FDS: %s for %.0f s: exiting when the last of %d player(s) leaves"), Why,
            FirstUnauthorizedAt >= 0.0 ? Now - FirstUnauthorizedAt : Now - LastReplyAt, Status.Players);
        bExitWhenEmpty = true;
    }
    // A long-running server's world clock is a float and its music clock an int32 (about 24 days), so after
    // FDS_MAX_UPTIME_HOURS (24; 0 = never) it is recycled in every mode: the status reports recycle_pending (fuserd
    // places no new players on it) and it exits once it is empty and in the Lobby; fuserd starts a fresh one as needed.
    static const double MaxUptimeSeconds = []
    {
        const FString Hours = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_MAX_UPTIME_HOURS"));
        return (Hours.IsEmpty() ? 24.0 : FCString::Atod(*Hours)) * 3600.0;
    }();
    if (!bRecyclePending && MaxUptimeSeconds > 0.0 && Now - GStartTime >= MaxUptimeSeconds)
    {
        bRecyclePending = true;
        bStatusDue = true;
        UE_LOG(LogFDS, Display, TEXT("FDS: up %.1f h: recycling once empty and in the Lobby (%d player(s) now)"), (Now - GStartTime) / 3600.0,
            Status.Players);
    }
    if (bRecyclePending && Status.Players == 0 && Status.Stage == static_cast<int32>(EMultiplayerGameplayStage::Lobby))
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: up %.1f h and empty: exiting for a scheduled restart"), (Now - GStartTime) / 3600.0);
        if (bOrphaned)
        {
            bExitSent = true;
        }
        ReportExit(TEXT("scheduled restart"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bExitWhenEmpty && bHadPlayers && Status.Players == 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: last player left: exiting"));
        if (bOrphaned)
        {
            bExitSent = true; // fuserd would not take the report
        }
        ReportExit(TEXT("last player left"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bInFlight)
    {
        return;
    }
    if (!bRegistered)
    {
        if (Now >= NextSendAt)
        {
            Send(TEXT("register"), RegistrationBody());
        }
    }
    else if (bChanged || bStatusDue || Now >= NextSendAt)
    {
        bStatusDue = false;
        Send(TEXT("status"), StatusBody(Status));
    }
}

void FFDSRegistration::ReportExit(const FString& Reason)
{
    if (!bEnabled || bExitSent)
    {
        return;
    }
    bExitSent = true;
    Send(TEXT("exit"), FString::Printf(TEXT("{\"slot_id\":%s,\"reason\":%s}"), *Quote(SlotId), *Quote(Reason)));
    // Shutting down: give the reply up to two seconds, ticking the HTTP manager ourselves.
    const double Deadline = FPlatformTime::Seconds() + 2.0;
    while (bInFlight && FPlatformTime::Seconds() < Deadline)
    {
        FHttpModule::Get().GetHttpManager().Tick(0.05f);
        FPlatformProcess::Sleep(0.05f);
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: reported exit to fuserd (%s)%s"), *Reason, bInFlight ? TEXT(" - no reply") : TEXT(""));
}
