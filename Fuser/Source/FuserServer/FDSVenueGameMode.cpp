#include "FDSVenueGameMode.h"
#include "FuserServer.h"
#include "FDSGameSession.h"
#include "FDSLobbyAuthority.h"
#include "FDSAdmission.h"
#include "FDSBeacon.h"
#include "FDSMainStage.h"
#include "FDSRegistration.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/OnlineReplStructs.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HypeGameStateMainStage.h"
#include "MainStageMPGameplayState.h"
#include "RequestMgr.h"
#include "HypeGameStateMultiplayerFreestyle.h"
#include "HypePlayerState.h"
#include "Net/OnlineEngineInterface.h"

AFDSVenueGameMode::AFDSVenueGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    // FUSER's BP_VenueGameMode CDO names HypeGameState; its native code selects the freestyle multiplayer state at run
    // time (HypeGameStateMultiplayerFreestyle); the Main Stage mode replaces it in InitGame.
    GameStateClass = AHypeGameStateMultiplayerFreestyle::StaticClass();
    PlayerStateClass = AHypePlayerState::StaticClass();
    GameSessionClass = AFDSGameSession::StaticClass();
    // The session authority's clock and stage driver run every frame.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

// The Main Stage mode (FDSMainStage): FUSER's Main Stage game state, whose MPS subobject is
// MainStageMPGameplayState, so clients spawn the Main Stage classes and run their overrides. The game state is spawned
// after InitGame (PreInitializeComponents), so the class is chosen here. The session holds FDS_MAX_PLAYERS (fuserd's
// main_stage.max_players) instead of FUSER's MaxPlayersInMainStage (300).
void AFDSVenueGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    if (FDSMainStage::IsEnabled())
    {
        GameStateClass = AHypeGameStateMainStage::StaticClass();
    }
    Super::InitGame(MapName, Options, ErrorMessage);
    if (FDSMainStage::IsEnabled() && GameSession)
    {
        const int32 Max = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_MAX_PLAYERS")));
        GameSession->MaxPlayers = Max > 0 && Max <= 300 ? Max : 50;
    }
}

void AFDSVenueGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (FDSMainStage::IsEnabled())
    {
        FDSMainStage::Tick();
    }
    if (Lobby)
    {
        Lobby->Tick(DeltaSeconds);
        FFDSRegistration::Get().Tick(DeltaSeconds, Lobby->GetStatus());
    }
}

void AFDSVenueGameMode::StartPlay()
{
    FDSAdmission::Init(); // session tokens
    Super::StartPlay();
    UE_LOG(LogFDS, Display, TEXT("FDS: game mode started; net mode=%d; map %s; game mode %s; game state %s; controller %s; pawn %s; HUD %s"),
        static_cast<int32>(GetWorld()->GetNetMode()), *GetWorld()->GetOutermost()->GetName(), *GetClass()->GetPathName(),
        GameState ? *GameState->GetClass()->GetPathName() : TEXT("none"), *GetPathNameSafe(PlayerControllerClass),
        *GetPathNameSafe(DefaultPawnClass), *GetPathNameSafe(HUDClass));
    UE_LOG(LogFDS, Display, TEXT("FDS: session capacity %d players (GameSession MaxPlayers)"), GameSession ? GameSession->MaxPlayers : -1);
    UMultiplayerGameplayState* Mps = nullptr;
    URequestMgr* Requests = nullptr;
    if (AHypeGameStateMultiplayerFreestyle* Freestyle = Cast<AHypeGameStateMultiplayerFreestyle>(GameState))
    {
        Mps = Freestyle->MultiplayerGameplayState;
        Requests = Freestyle->RequestMgr;
    }
    else if (AHypeGameStateMainStage* MainStage = Cast<AHypeGameStateMainStage>(GameState))
    {
        Mps = MainStage->MultiplayerGameplayState;
        Requests = MainStage->RequestMgr;
    }
    if (Mps)
    {
        Lobby = NewObject<UFDSLobbyAuthority>(this);
        Lobby->Start(Cast<AHypeGameState>(GameState), Mps, Requests, FDSMainStage::IsEnabled());
        FFDSRegistration::Get().Start(GetWorld()); // fuserd: registers on the next tick
        FDSBeacon::Start(GetWorld(), Lobby);       // the session browser's beacon on the game port + 100
    }
}

void AFDSVenueGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    FFDSRegistration::Get().ReportExit(TEXT("shutdown"));
    if (Lobby)
    {
        Lobby->Stop();
    }
    Super::EndPlay(Reason);
}

// FUN_140707a50 (FUSER's host-side PostLogin storage) reads playerSessionId / gameSessionId / hypeUserId /
// buildConfig from the login URL. FDS keeps the user id and game session id on the PlayerState; values never logged.
FString AFDSVenueGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
    const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
    FDSAdmission::Admitted(Options, NewPlayerController);
    if (AHypePlayerState* Player = NewPlayerController ? NewPlayerController->GetPlayerState<AHypePlayerState>() : nullptr)
    {
        const FString HypeUserId = UGameplayStatics::ParseOption(Options, TEXT("hypeUserId"));
        const FString GameSessionId = UGameplayStatics::ParseOption(Options, TEXT("gameSessionId"));
        if (!HypeUserId.IsEmpty())
        {
            Player->_hypeUserId = FCString::Atoi(*HypeUserId);
        }
        if (!GameSessionId.IsEmpty())
        {
            Player->_gameSessionId = GameSessionId;
        }
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d identity from login: hype user id %s, game session id %s"), Player->PlayerId,
            HypeUserId.IsEmpty() ? TEXT("absent") : TEXT("present"), GameSessionId.IsEmpty() ? TEXT("absent") : TEXT("present"));
    }
    return Error;
}

void AFDSVenueGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    // Stock AGameModeBase::PreLogin accepts only ids of the loaded default online subsystem's type. FUSER servers run
    // with DefaultPlatformService=Hype, so FUSER clients present "Hype" ids; FDS has no Hype subsystem (its default is
    // NULL). Accept the Hype type explicitly as well as the local default (matching-project test clients); other
    // types are still rejected. Session tokens are checked by FDSAdmission.
    static const FName HypeIdType(TEXT("Hype"));
    const bool bIdTypeOk = !UniqueId.IsValid() || UniqueId.GetType() == HypeIdType
        || UniqueId.GetType() == UOnlineEngineInterface::Get()->GetDefaultOnlineSubsystemName();
    ErrorMessage = bIdTypeOk ? GameSession->ApproveLogin(Options) : FString(TEXT("incompatible_unique_net_id"));
    // fuserd session tokens (6.6, FDSAdmission): the reason is logged, the token never.
    FDSAdmission::Init();
    if (ErrorMessage.IsEmpty() && FDSMainStage::IsEnabled())
    {
        // The Main Stage join carries no session token (FDSMainStage): the pass is checked with fuserd after login (PostLogin).
        if (FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("hypeUserId"))) <= 0)
        {
            ErrorMessage = TEXT("invalid_player_session");
            UE_LOG(LogFDS, Warning, TEXT("FDS: login refused: main stage login without a hype user id"));
        }
    }
    else if (ErrorMessage.IsEmpty())
    {
        const FString Refused = FDSAdmission::Check(Options);
        if (!Refused.IsEmpty())
        {
            ErrorMessage = TEXT("invalid_player_session");
            UE_LOG(LogFDS, Warning, TEXT("FDS: login refused: %s"), *Refused);
        }
    }
    FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);

    // Option names and value lengths only: the values carry session tokens and user identifiers.
    TArray<FString> Pairs;
    Options.ParseIntoArray(Pairs, TEXT("?"), true);
    FString Keys;
    for (const FString& Pair : Pairs)
    {
        FString Key, Value;
        if (!Pair.Split(TEXT("="), &Key, &Value))
        {
            Key = Pair;
        }
        Keys += FString::Printf(TEXT("%s%s(%d)"), Keys.IsEmpty() ? TEXT("") : TEXT(","), *Key, Value.Len());
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: prelogin options [%s], unique id %s (type %s), result %s"), *Keys,
        UniqueId.IsValid() ? TEXT("valid") : TEXT("invalid"), *UniqueId.GetType().ToString(),
        ErrorMessage.IsEmpty() ? TEXT("accepted") : *ErrorMessage);
}

void AFDSVenueGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (FDSMainStage::IsEnabled() && NewPlayer)
    {
        const AHypePlayerState* Identity = NewPlayer->GetPlayerState<AHypePlayerState>();
        FDSMainStage::CheckPass(NewPlayer, Identity ? Identity->_hypeUserId : 0);
    }
    if (Lobby && NewPlayer)
    {
        if (AHypePlayerState* Player = NewPlayer->GetPlayerState<AHypePlayerState>())
        {
            Lobby->OnPlayerJoined(Player);
        }
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: client admitted; player_state=%s; controller %s; player state %s; pawn %s"),
        NewPlayer && NewPlayer->PlayerState ? TEXT("present") : TEXT("absent"), *GetPathNameSafe(NewPlayer ? NewPlayer->GetClass() : nullptr),
        *GetPathNameSafe(NewPlayer && NewPlayer->PlayerState ? NewPlayer->PlayerState->GetClass() : nullptr),
        *GetPathNameSafe(NewPlayer && NewPlayer->GetPawn() ? NewPlayer->GetPawn()->GetClass() : nullptr));
}

void AFDSVenueGameMode::Logout(AController* Exiting)
{
    UE_LOG(LogFDS, Display, TEXT("FDS: client disconnected"));
    FDSAdmission::Left(Exiting);
    FDSMainStage::Forget(Cast<APlayerController>(Exiting));
    if (Lobby && Exiting)
    {
        if (AHypePlayerState* Player = Exiting->GetPlayerState<AHypePlayerState>())
        {
            Lobby->OnPlayerLeft(Player);
        }
    }
    Super::Logout(Exiting);
}

APawn* AFDSVenueGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
    // ShellPawn's CDO does not replicate, yet FUSER's pawns replicate to their owner (BP_HypePawn: owner-only, no
    // movement replication): replication is switched on for the spawned pawn.
    if (Pawn)
    {
        Pawn->SetReplicates(true);
    }
    return Pawn;
}
