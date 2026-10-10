// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "HypeActorComponent.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "RequestMgr.generated.h"



// /Script/Hype.RequestMgr (parent /Script/Hype.HypeActorComponent)
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API URequestMgr : public UHypeActorComponent
{
    GENERATED_BODY()

public:
    URequestMgr(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 416, flags 0x0040000100002220
    UPROPERTY(Transient, ReplicatedUsing=OnRep_MultiplayerLiveRequests)
    TArray<FMultiplayerLiveRequest> _multiplayerLiveRequests;


    UFUNCTION()
    void OnRep_MultiplayerLiveRequests();

};
