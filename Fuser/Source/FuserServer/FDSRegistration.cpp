#include "FDSRegistration.h"
#include "FDSMainStage.h"
#include "FDSVenues.h"
#include "FuserServer.h"
#include "Dom/JsonObject.h"
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
}

FFDSRegistration::FFDSRegistration()
{
    Url = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_REGISTER_URL")).TrimStartAndEnd();
    SlotId = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_SLOT_ID")).TrimStartAndEnd();
    Token = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_REGISTER_TOKEN")).TrimStartAndEnd();
    const int32 Heartbeat = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_HEARTBEAT_SECONDS")));
    HeartbeatSeconds = Heartbeat > 0 ? Heartbeat : 10.0f;
    bExitWhenEmpty = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_EXIT_WHEN_EMPTY")) != TEXT("0") && !FDSMainStage::IsEnabled();
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
}

FString FFDSRegistration::RegistrationBody() const
{
    const int32 Port = World.IsValid() ? World->URL.Port : 0;
    const FDSVenues::FSelection& Venue = FDSVenues::Selected();
    return FString::Printf(TEXT("{\"slot_id\":%s,\"version\":%s,\"net_version\":%d,\"port\":%d,\"beacon_port\":%d,\"venue\":%s,\"map\":%s,")
        TEXT("\"time_of_day\":%s,\"rounds\":%d,\"turn_measures\":%d}"),
        *Quote(SlotId), *Quote(Version), NetVersion, Port, Port + 100, *Quote(Venue.Id), *Quote(Venue.Map), *Quote(Venue.TimeOfDay),
        Last.Rounds, Last.TurnMeasures);
}

// Members are hype user ids with their team (dj / spectator / unknown), in join order; current_dj is the turn's DJ
// (0 = none) and event the Main Stage event being played ("" = none).
FString FFDSRegistration::StatusBody(const FFDSLobbyStatus& Status) const
{
    FString Members;
    for (const TPair<int32, EMultiplayerTeam>& Member : Status.Members)
    {
        const TCHAR* Team = Member.Value == EMultiplayerTeam::DJ ? TEXT("dj") : Member.Value == EMultiplayerTeam::Spectator ? TEXT("spectator") : TEXT("unknown");
        Members += FString::Printf(TEXT("%s{\"user_id\":%d,\"team\":\"%s\"}"), Members.IsEmpty() ? TEXT("") : TEXT(","), Member.Key, Team);
    }
    return FString::Printf(TEXT("{\"slot_id\":%s,\"players\":%d,\"djs\":%d,\"viewers\":%d,\"stage\":%d,\"round\":%d,\"members\":[%s],")
        TEXT("\"current_dj\":%d,\"event\":%s}"), *Quote(SlotId), Status.Players, Status.DJs, Status.Viewers, Status.Stage, Status.Round, *Members,
        Status.CurrentDJ, *Quote(Status.Event));
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

void FFDSRegistration::OnReply(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk, FString Call)
{
    bInFlight = false;
    const int32 Code = bOk && Response.IsValid() ? Response->GetResponseCode() : 0;
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
    // The Main Stage server runs for days; its world clock is a float and its music clock an int32 (about 24 days),
    // so after FDS_MAX_UPTIME_HOURS (24; 0 = never) it exits once it is empty and in the Lobby, and fuserd starts a
    // fresh one.
    static const double MaxUptimeSeconds = []
    {
        const FString Hours = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_MAX_UPTIME_HOURS"));
        return (Hours.IsEmpty() ? 24.0 : FCString::Atod(*Hours)) * 3600.0;
    }();
    if (FDSMainStage::IsEnabled() && MaxUptimeSeconds > 0.0 && FPlatformTime::Seconds() - GStartTime >= MaxUptimeSeconds &&
        Status.Players == 0 && Status.Stage == static_cast<int32>(EMultiplayerGameplayStage::Lobby))
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: main stage: up %.1f h and empty: exiting for a scheduled restart"), (FPlatformTime::Seconds() - GStartTime) / 3600.0);
        ReportExit(TEXT("scheduled restart"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bExitWhenEmpty && bHadPlayers && Status.Players == 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: last player left: exiting"));
        ReportExit(TEXT("last player left"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bInFlight)
    {
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (!bRegistered)
    {
        if (Now >= NextSendAt)
        {
            Send(TEXT("register"), RegistrationBody());
        }
    }
    else if (bChanged || Now >= NextSendAt)
    {
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
