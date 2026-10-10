// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "HypeActorComponent.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "ResultsState.generated.h"



// /Script/Hype.ResultsState (parent /Script/Hype.HypeActorComponent)
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API UResultsState : public UHypeActorComponent
{
    GENERATED_BODY()

public:
    UResultsState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 192, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_Results)
    FGameplayResults _results;


    // FUSER offset 560, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_MainStageResults)
    FMainStageResults _mainStageResults;


    UFUNCTION()
    void OnRep_Results();


    UFUNCTION()
    void OnRep_MainStageResults();

};
