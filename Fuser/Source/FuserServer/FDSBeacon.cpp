#include "FDSBeacon.h"
#include "FDSAssetPlaceholders.h"
#include "FDSLobbyAuthority.h"
#include "FDSRegistration.h"
#include "FDSVenues.h"
#include "FuserServer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "HypeMultiplayerBeacon.h"
#include "HypePlayerState.h"
#include "VenueData.h"

namespace
{
    TWeakObjectPtr<AHypeMultiplayerBeaconHost> GHost;
    TWeakObjectPtr<UFDSLobbyAuthority> GLobby;
    TWeakObjectPtr<UWorld> GBeaconWorld;
    double GStartedAt = 0.0;
    int32 GReplies = 0;

    EVenueTimeOfDay TimeOfDay(const FString& Name)
    {
        return Name == TEXT("day") ? EVenueTimeOfDay::Day : Name == TEXT("sunset") ? EVenueTimeOfDay::Sunset
            : Name == TEXT("night") ? EVenueTimeOfDay::Night : EVenueTimeOfDay::Morning;
    }

    void Fill(FGameSessionMetadata& M)
    {
        M.gameSessionId = FFDSRegistration::Get().SessionId();
        M.GameMode = EHypeGameMode::MultiplayerFreestyle;
        M.BrowserEntrySource = ESessionBrowserEntrySource::Unknown;
        M.SessionDurationSeconds = static_cast<float>(FPlatformTime::Seconds() - GStartedAt);
        M.IPAdress = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_ADDRESS")).TrimStartAndEnd();
        M.Port = GBeaconWorld.IsValid() ? FString::FromInt(GBeaconWorld->URL.Port) : FString();
        if (UFDSLobbyAuthority* Lobby = GLobby.Get())
        {
            const FFDSLobbyStatus Status = Lobby->GetStatus();
            M.NumDJs = Status.DJs;
            M.NumSpectators = Status.Viewers;
            M.FreestyleTurnLengthMeasures = Status.TurnMeasures;
            M.FreestyleTotalRoundNumber = Status.Rounds;
            M.CurrentState = static_cast<EMultiplayerGameplayStage>(Status.Stage);
            M.IsPrivateSession = !Lobby->IsPublicSession();
        }
        if (GBeaconWorld.IsValid() && GBeaconWorld->GetGameState())
        {
            for (APlayerState* Each : GBeaconWorld->GetGameState()->PlayerArray)
            {
                const AHypePlayerState* Player = Cast<AHypePlayerState>(Each);
                if (Player && Player->_isSessionLeader)
                {
                    M.LeaderHypeUserId = Player->_hypeUserId;
                    M.LeaderUserName = Player->GetPlayerName();
                }
            }
        }
        const FDSVenues::FSelection& Venue = FDSVenues::Selected();
        if (!Venue.Id.IsEmpty())
        {
            // FUSER's catalogue path: /Game/Gameplay/Venues/<Id>.<Id>
            M.venue = Cast<UVenueData>(FDSAssetPlaceholders::GetOrCreate(TEXT("/Game/Gameplay/Venues/") + Venue.Id, FName(*Venue.Id),
                UVenueData::StaticClass()));
        }
        M.VenueTimeOfDay = TimeOfDay(Venue.TimeOfDay);
        if (++GReplies <= 3 || GReplies % 50 == 0)
        {
            UE_LOG(LogFDS, Display, TEXT("FDS: beacon reply #%d: session %s, %s, %d DJ(s) %d viewer(s), stage %d, venue %s"), GReplies,
                M.gameSessionId.IsEmpty() ? TEXT("(no fuserd session yet)") : *M.gameSessionId, M.IsPrivateSession ? TEXT("private") : TEXT("public"),
                M.NumDJs, M.NumSpectators, static_cast<int32>(M.CurrentState), Venue.Id.IsEmpty() ? TEXT("(none)") : *Venue.Id);
        }
    }
}

void FDSBeacon::Start(UWorld* World, UFDSLobbyAuthority* Lobby)
{
    if (!World || GHost.IsValid() || (World->GetNetMode() != NM_DedicatedServer && World->GetNetMode() != NM_ListenServer))
    {
        return; // servers only (not a standalone test client)
    }
    GBeaconWorld = World;
    GLobby = Lobby;
    GStartedAt = FPlatformTime::Seconds();
    AHypeMultiplayerBeaconHostObject::MetadataProvider = &Fill;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    AHypeMultiplayerBeaconHost* Host = World->SpawnActor<AHypeMultiplayerBeaconHost>(Params);
    if (!Host)
    {
        UE_LOG(LogFDS, Error, TEXT("FDS: beacon host not spawned: the session browser won't list this server"));
        return;
    }
    Host->ListenPort = World->URL.Port + 100; // FUSER's beacon is always the game port + 100 (BeaconPort= still overrides)
    if (!Host->InitHost())
    {
        UE_LOG(LogFDS, Error, TEXT("FDS: beacon host failed to listen on UDP %d: the session browser won't list this server"), Host->ListenPort);
        Host->Destroy();
        return;
    }
    AHypeMultiplayerBeaconHostObject* HostObject = World->SpawnActor<AHypeMultiplayerBeaconHostObject>(Params);
    Host->RegisterHost(HostObject);
    Host->PauseBeaconRequests(false);
    GHost = Host;
    UE_LOG(LogFDS, Display, TEXT("FDS: session-browser beacon listening on UDP %d (type %s)"), Host->GetListenPort(), *HostObject->GetBeaconType());
}
