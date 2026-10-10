// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "HypePlayerController.generated.h"



// /Script/Hype.HypePlayerController (parent /Script/Engine.PlayerController)
UCLASS(Blueprintable)
class HYPE_API AHypePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AHypePlayerController(const FObjectInitializer& ObjectInitializer);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiMessageChance(float chance);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamEnabled(bool Enabled);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamOnBeat();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamRandomStreak();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamRate(float perSec);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamStreak(int32 Streak);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamType(EMultiplayerPraiseType Type);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SetRhythmojiSpamTypeRandom();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SubmitRequestIn(float Time);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_AutoSpec_SubmitRequestRandom(float Time);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerDumpObjList(const FString& ClassName);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void CHEAT_ServerEnableStatReplicator(bool Enabled);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerSetQueryStatsEnabled(bool Enabled);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerSetTickRate(int32 tickRate);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerStartNetProfiling();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerStartStat(const FString& baseFileName);


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerStopNetProfiling();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void CHEAT_ServerStopStat();


    // FUSER flags 0x80220CC0
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdatePlayerAfkSeconds(int32 afkSeconds);


    // FUSER flags 0x01040CC1
    UFUNCTION(Client, Reliable)
    void _ClientReportServerTime(float requestWorldTime, float serverTime);


    // FUSER flags 0x80240CC1
    UFUNCTION(Server, Reliable, WithValidation)
    void _ServerRequestServerTime(float requestWorldTime);

};
