#pragma once

#include "CoreMinimal.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"

class AHypePlayerState;
class UWorld;

// Hand-written (not generated). The Hype module reproduces FUSER's network surface; server decisions that FUSER's
// native code took in UMultiplayerGameplayState / UHypeMultiplayerServerBase (team rules, readiness, leadership, the
// stage machine) belong to the FDS server, which registers an implementation per world (FuserServer
// UFDSLobbyAuthority). RPC bodies in Private/HypeServerRpcs.cpp forward to it; native references are recorded there.
class HYPE_API IHypeServerAuthority
{
public:
    virtual ~IHypeServerAuthority() = default;

    virtual void OnSetHypeUserId(AHypePlayerState* Player) = 0;
    virtual void OnSetIsReadyInLobby(AHypePlayerState* Player, bool bIsReady) = 0;
    virtual void OnSetIsReadyInResults(AHypePlayerState* Player, bool bIsReady) = 0;
    virtual void OnRequestTeamChange(AHypePlayerState* Player, EMultiplayerTeam NewTeam) = 0;
    virtual void OnMarkLobbyComplete(AHypePlayerState* Player) = 0;
    virtual void OnChangeLeaderTo(AHypePlayerState* Caller, AHypePlayerState* NewLeader) = 0;
    virtual void OnUpdateAfkSeconds(class AHypePlayerController* Controller, int32 AfkSeconds) = 0;
    virtual void OnSendSignatureMove(AHypePlayerState* Player) = 0;
    virtual void OnLeaderKickPlayer(AHypePlayerState* Leader, AHypePlayerState* Target) = 0;
    virtual void OnToggleAfkChecks(AHypePlayerState* Player) = 0;
    // A DJ action RPC, as the action item listeners replay (UniqueId assigned by the authority).
    virtual void OnRecordAction(AHypePlayerState* Player, FMultiplayerActionItemEntry& Item) = 0;
    // An effect / instrument parameter update, as an item of the sender's _multiplayerFXParams / _multiplayerInstrumentParams.
    virtual void OnRecordParam(AHypePlayerState* Player, const FMultiplayerUnreliableActionItemEntry& Item) = 0;
    // Crafted instruments: a DJ's new instrument (registered and sent to the others) and a listener's request for one.
    virtual void OnAddCraftedInstrument(AHypePlayerState* Player, const FSerializableCraftedInstrumentCel& Data) = 0;
    virtual void OnRequestCraftedInstrument(AHypePlayerState* Player, FName ShortName) = 0;
    // Reactions: praises (both RPC variants share FUSER's body) and snapshots.
    virtual void OnSendPraises(AHypePlayerState* Sender, const TArray<FCollectedPraise>& Praises) = 0;
    virtual void OnSendSnapshotTaken(AHypePlayerState* Sender, AHypePlayerState* ToPlayer, int32 AtMeasure) = 0;
    // A viewer's live request to a DJ.
    virtual void OnSubmitRequest(AHypePlayerState* Sender, const FMultiplayerLiveRequest& Request, AHypePlayerState* ToDJ) = 0;
    virtual void OnUpdateGlobalAudioState(AHypePlayerState* Player, int32 Tick, int32 BPM, EKey Key, EKeyMode Mode) = 0;
    virtual void OnSetCoopRoundNumTurnMeasures(AHypePlayerState* Player, int32 NumRounds, int32 TurnMeasures) = 0;

    static void Register(UWorld* World, IHypeServerAuthority* Authority);
    static void Unregister(UWorld* World, IHypeServerAuthority* Authority);
    static IHypeServerAuthority* Get(const UObject* WorldContext);
};
