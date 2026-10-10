#pragma once

#include "CoreMinimal.h"
#include "VenueGameMode.h"
#include "FDSVenueGameMode.generated.h"

// FDS's server game mode: the native parent of FDS's /Game/GameFramework/BP_VenueGameMode stub, so clients receive
// FUSER's game-mode class path (GameState.GameModeClass). Class defaults reproduce FUSER's runtime selection:
// HypeGameStateMultiplayerFreestyle, HypePlayerState; the blueprint stub's CDO sets the blueprint
// classes (BP_HypePlayerController, BP_HypePawn, BP_HypeHUD) as FUSER's BP_VenueGameMode CDO does.
// No game-specific RPCs or replicated members may be added here: the class is never replicated, but keep it that way.
UCLASS()
class FUSERSERVER_API AFDSVenueGameMode : public AVenueGameMode
{
    GENERATED_BODY()

public:
    AFDSVenueGameMode(const FObjectInitializer& ObjectInitializer);

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
    virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY()
    class UFDSLobbyAuthority* Lobby = nullptr;
};
