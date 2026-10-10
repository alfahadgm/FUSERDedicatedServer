// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeEnums.h"
#include "HypeNetTypes.h"
#include "DJPlayerState.generated.h"



// /Script/Hype.DJPlayerState (parent /Script/Engine.ActorComponent)
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API UDJPlayerState : public UActorComponent
{
    GENERATED_BODY()

public:
    UDJPlayerState(const FObjectInitializer& ObjectInitializer);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // FUSER offset 832, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_AvatarConfig)
    FNetAvatarConfig _netAvatarConfig;


    // FUSER offset 920, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_VenueConfig)
    FVenueConfigKeys _venueConfig;


    // FUSER offset 1024, flags 0x0040000100002020
    UPROPERTY(Transient, ReplicatedUsing=OnRep_CrateData)
    FCrateData _crateData;


    UFUNCTION()
    void OnRep_AvatarConfig();


    UFUNCTION()
    void OnRep_VenueConfig();


    UFUNCTION()
    void OnRep_CrateData();


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetAvatarConfig(const FNetAvatarConfig& NetAvatarConfig);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetCrate(const FCrateData& CrateData);


    // FUSER flags 0x00220CC0
    UFUNCTION(Server, Reliable)
    void ServerSetVenueConfig(const FVenueConfigKeys& VenueConfig);

};
