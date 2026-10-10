// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "HypePlayerState.generated.h"

class UDJPlayerState;
class UInstrumentCelData;
class UResultsState;


// /Script/Hype.HypePlayerState (parent /Script/Engine.PlayerState)
UCLASS(Blueprintable)
class HYPE_API AHypePlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AHypePlayerState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 1288, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_HypeUserId)
    int32 _hypeUserId;


    // FUSER offset 1296, flags 0x0048000100002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_PlatformUserId)
    FString _platformUserId;


    // FUSER offset 1329, flags 0x0048001040002220
    UPROPERTY(Transient, Replicated)
    EHypeOnlinePlatform _platform;


    // FUSER offset 1332, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_XP)
    int32 _hypeXP;


    // FUSER offset 1336, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_Team)
    EMultiplayerTeam _team;


    // FUSER offset 1340, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_DJIndex)
    int32 _djIndex;


    // FUSER offset 1352, flags 0x0040000100000020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_MultiplayerActions)
    FMultiplayerActionArray _multiplayerActions;


    // FUSER offset 1632, flags 0x0040000100000020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_MultiplayerFXParams)
    FMultiplayerUnreliableActionArray _multiplayerFXParams;


    // FUSER offset 1912, flags 0x0040000100000020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_MultiplayerInstrumentParams)
    FMultiplayerUnreliableActionArray _multiplayerInstrumentParams;


    // FUSER offset 2192, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_StateOfTheWorld)
    FStateOfTheWorld _stateOfTheWorld;


    // FUSER offset 4168, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_IsSessionLeader)
    bool _isSessionLeader;


    // FUSER offset 4169, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_IsReadyCheck)
    EHypePlayerStateIsReadyFlags _isReadyCheck;


    // FUSER offset 4176, flags 0x0048000100002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_GameSessionId)
    FString _gameSessionId;


    // FUSER offset 4192, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_ServerAccessState)
    EServerAccessState _serverAccessState;


    // FUSER offset 4216, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_VoiceChatGroup)
    int32 _mainStageVoiceChatGroupID;


    // FUSER offset 4264, flags 0x0048001140080228
    UPROPERTY(Transient, ReplicatedUsing=OnRep_DJPlayerState)
    UDJPlayerState* _djPlayerState;


    // default subobject "ResultsState"
    UPROPERTY(VisibleAnywhere)
    UResultsState* _resultsState;


    UFUNCTION()
    void OnRep_HypeUserId();


    UFUNCTION()
    void OnRep_PlatformUserId();


    UFUNCTION()
    void OnRep_XP();


    UFUNCTION()
    void OnRep_Team();


    UFUNCTION()
    void OnRep_DJIndex();


    UFUNCTION()
    void OnRep_MultiplayerActions();


    UFUNCTION()
    void OnRep_MultiplayerFXParams();


    UFUNCTION()
    void OnRep_MultiplayerInstrumentParams();


    UFUNCTION()
    void OnRep_StateOfTheWorld();


    UFUNCTION()
    void OnRep_IsSessionLeader();


    UFUNCTION()
    void OnRep_IsReadyCheck();


    UFUNCTION()
    void OnRep_GameSessionId();


    UFUNCTION()
    void OnRep_ServerAccessState();


    UFUNCTION()
    void OnRep_VoiceChatGroup();


    UFUNCTION()
    void OnRep_DJPlayerState();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_ServerForceNumRoundsMeasures(int32 rounds, int32 measures);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_ServerRequestMultiplayerGameStart(float startIn);


    // FUSER flags 0x01020CC0
    UFUNCTION(Client, Reliable)
    void ClientSendCraftedInstrumentCel(const FSerializableCraftedInstrumentCel& Data);


    // FUSER flags 0x01020CC0
    UFUNCTION(Client, Reliable)
    void ClientSendDataPair(const FString& Key, const FString& Value);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAddCraftedInstrumentCel(const FSerializableCraftedInstrumentCel& Data);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerBeginInstrumentCrafting(int32 Tick, EInstrumentCraftingSlot slot, UInstrumentCelData* InstrumentCelData);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerChangeLeaderTo(AHypePlayerState* ps);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerDoDropQueue(int32 Tick);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerEjectQueue(int32 Tick, ETrackType trackType);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerEjectTrack(int32 Tick, ETrackType trackType);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerEndInstrumentCrafting(int32 Tick, EInstrumentCraftingSlot slot);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerHotSwapOnDeck(int32 Tick, ETrackType trackType);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerLeaderKickPlayer(AHypePlayerState* ps);


    // FUSER flags 0x84220CC0
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
    void ServerMarkLobbyComplete();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestCraftedInstrumentCel(FName instrumentCelShortName);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestMultiplayerTeamChange(EMultiplayerTeam newTeam);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendDataPair(const FString& Key, const FString& Value);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendPraises(const TArray<FCollectedPraise>& praises);


    // FUSER flags 0x80220C40
    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerSendPraises_Unreliable(const TArray<FCollectedPraise>& praises);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendSignatureMove();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendSnapshotTaken(AHypePlayerState* toPlayer, int32 atMeasure);


    // FUSER flags 0x84220CC0
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
    void ServerSetAsDJ();


    // FUSER flags 0x84220CC0
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
    void ServerSetAsSpectator();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetBattleMatchmakingRating(int32 battleMatchmakingRating);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetBattleNumDiscs(int32 battleNumDiscs);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetCoopRoundNumTurnMeasures(int32 numRounds, int32 turnMeasures);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetDLCDownloadState(EDLCDownloadState downloadState);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetGainForTrack(int32 Tick, ETrackType trackType, float gain);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetHypeUserId(int32 hypeUserId);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetInitialBlockedHypeUserIds(const TArray<int32>& initialBlockedHypeUsers);


    // FUSER flags 0x84220CC0
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
    void ServerSetIsReadyInLobby(bool IsReady);


    // FUSER flags 0x84220CC0
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
    void ServerSetIsReadyInResults(bool IsReady);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetNumBattleMatchesLost(int32 battleMatchesLost);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetNumBattleMatchesPlayed(int32 battleMatchesPlayed);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetNumBattleMatchesWon(int32 battleMatchesWon);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetPlatform(EHypeOnlinePlatform platform);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetPlatformUserId(const FString& platformUserId);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetXP(int32 xp);


    // FUSER flags 0x80A20CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSpawnDisc(int32 Tick, FName celShortName, ETrackType trackType, const FTransform& spawnTransform, EActionAfterDiscSpawn actionAfterDiscSpawn, int32 dropStartTick, int32 dropEndTick, bool isBattleSuperDrop);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSubmitRequest(const FMultiplayerLiveRequest& req, AHypePlayerState* toDJ);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggleAFKChecks();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggleAutoReadyCheck();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTogglePinDJ();


    // FUSER flags 0x80220C40
    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerUpdateAudioEffectParams(int32 Tick, EAudioEffectSlot slot, float paramX, float paramY);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateAudioEffectState(int32 Tick, EAudioEffectSlot slot, const FAudioEffectInfo& Info);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateGlobalAudioState(int32 Tick, int32 BPM, EKey Key, EKeyMode mode);


    // FUSER flags 0x80220C40
    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerUpdateInstrumentCraftingState(int32 Tick, EInstrumentCraftingSlot slot, const FGridInstrumentCraftingState& newState);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateRequestCompletion(int32 Tick, int32 uniqueRequestId, EMultiplayerLiveRequestCompletionType completionType);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateSoloMuteState(int32 Tick, const TArray<FDeckMuteSoloState>& soloMuteDeckStates);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateVoiceChatGroup(bool isSpectatorVoiceChatPermitted);


    // FUSER flags 0x80240CC1
    UFUNCTION(Server, Reliable, WithValidation)
    void _ServerSetHasSyncedStateOfTheWorld(bool synced);

};
