// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "HypeGameStateVenue.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "HypeGameStateMultiplayerFreestyle.generated.h"

class UMultiplayerGameplayState;
class URequestMgr;


// /Script/Hype.HypeGameStateMultiplayerFreestyle (parent /Script/Hype.HypeGameStateVenue)
UCLASS(Blueprintable)
class HYPE_API AHypeGameStateMultiplayerFreestyle : public AHypeGameStateVenue
{
    GENERATED_BODY()

public:
    AHypeGameStateMultiplayerFreestyle(const FObjectInitializer& ObjectInitializer);


    // default subobject "MultiplayerGameplayState"
    UPROPERTY(VisibleAnywhere)
    UMultiplayerGameplayState* MultiplayerGameplayState;


    // default subobject "RequestMgr"
    UPROPERTY(VisibleAnywhere)
    URequestMgr* RequestMgr;

};
