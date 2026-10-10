// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "HypeGameState.generated.h"

class AHypePlayerState;
class UMusicTimelineState;


// /Script/Hype.HypeGameState (parent /Script/Engine.GameStateBase)
UCLASS(Blueprintable)
class HYPE_API AHypeGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AHypeGameState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 1176, flags 0x0040001000002020
    UPROPERTY(Transient, Replicated)
    FAutospectatorSettings _autoSpectatorSettings;


    // default subobject "MusicTimelineState"
    UPROPERTY(VisibleAnywhere)
    UMusicTimelineState* MusicTimelineState;


    // FUSER flags 0x00024CC0
    UFUNCTION(NetMulticast, Reliable)
    void NetMulticastPraises(const TArray<FCollectedPraise>& praises);


    // FUSER flags 0x00024C40
    UFUNCTION(NetMulticast, Unreliable)
    void NetMulticastPraisesUnreliable(const TArray<FCollectedPraise>& praises);


    // FUSER flags 0x00024CC0
    UFUNCTION(NetMulticast, Reliable)
    void NetMulticastSignatureMove(AHypePlayerState* FromPlayer);


    // FUSER flags 0x00024CC0
    UFUNCTION(NetMulticast, Reliable)
    void NetMulticastSuperDropPerformed(EBattlePlayerLabel battlePlayerLabel);

};
