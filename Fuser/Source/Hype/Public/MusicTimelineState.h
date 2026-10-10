// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "MusicTimelineState.generated.h"



// /Script/Hype.MusicTimelineState (parent /Script/Engine.ActorComponent)
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API UMusicTimelineState : public UActorComponent
{
    GENERATED_BODY()

public:
    UMusicTimelineState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 224, flags 0x0040001100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_SyncState)
    FReplicatedTimelineSynchronizationState _syncState;


    UFUNCTION()
    void OnRep_SyncState();

};
