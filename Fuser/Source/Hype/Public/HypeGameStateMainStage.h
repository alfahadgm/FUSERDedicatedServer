// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "HypeGameStateVenue.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "HypeGameStateMainStage.generated.h"

class UMainStageMPGameplayState;
class URequestMgr;


// /Script/Hype.HypeGameStateMainStage (parent /Script/Hype.HypeGameStateVenue)
UCLASS(Blueprintable)
class HYPE_API AHypeGameStateMainStage : public AHypeGameStateVenue
{
    GENERATED_BODY()

public:
    AHypeGameStateMainStage(const FObjectInitializer& ObjectInitializer);


    // default subobject "MainStageMPGameplayState"
    UPROPERTY(VisibleAnywhere)
    UMainStageMPGameplayState* MultiplayerGameplayState;


    // default subobject "RequestMgr"
    UPROPERTY(VisibleAnywhere)
    URequestMgr* RequestMgr;

};
