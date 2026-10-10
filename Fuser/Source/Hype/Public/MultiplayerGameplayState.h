// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "MultiplayerGameplayState.generated.h"



// /Script/Hype.MultiplayerGameplayState (parent /Script/Engine.ActorComponent)
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API UMultiplayerGameplayState : public UActorComponent
{
    GENERATED_BODY()

public:
    UMultiplayerGameplayState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 616, flags 0x0010000100002234
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_MainStageRhythmojiRates)
    TArray<float> MainStageRhythmojiRates;


    // FUSER offset 656, flags 0x0020080100002220
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_GameplayStages)
    TArray<FMultiplayerGameStage> _gameStages;


    // FUSER offset 676, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_NumCoopFreestyleRounds)
    int32 _numCoopFreestyleRounds;


    // FUSER offset 680, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_CoopTurnLengthMeasures)
    int32 _coopTurnLengthMeasures;


    // FUSER offset 688, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_HottestMix)
    FMultiplayerMeasureHistoryEntry _hottestMix;


    // FUSER offset 760, flags 0x0048001140002220
    UPROPERTY(Transient, ReplicatedUsing=_OnRep_ServerShutdownWorldTime)
    float _serverShutdownWorldTime;


    // FUSER offset 1060, flags 0x0048001040002220
    UPROPERTY(Transient, Replicated)
    bool _pinCurrentStage;


    UFUNCTION()
    void _OnRep_MainStageRhythmojiRates();


    UFUNCTION()
    void _OnRep_GameplayStages();


    UFUNCTION()
    void _OnRep_NumCoopFreestyleRounds();


    UFUNCTION()
    void _OnRep_CoopTurnLengthMeasures();


    UFUNCTION()
    void _OnRep_HottestMix();


    UFUNCTION()
    void _OnRep_ServerShutdownWorldTime();

};
