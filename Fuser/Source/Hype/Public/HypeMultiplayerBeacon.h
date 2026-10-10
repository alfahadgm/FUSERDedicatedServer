#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "OnlineBeaconHost.h"
#include "OnlineBeaconHostObject.h"
#include "HypeEnums.h"
#include "HypeMultiplayerBeacon.generated.h"

class UVenueData;

// Hand-written (not generated: the recorded network metadata doesn't cover the beacon). FUSER's session-browser
// beacon: a client connects to the game port + 100 with a HypeMultiplayerBeaconClient (stock UE4
// beacon join; the beacon type is the client class name), the host answers ClientSendGameSessionMetadata with the
// session's FGameSessionMetadata (FUSER builds it in FUN_140708b10) and the client lists the session.
// Layouts from FUSER's reflection data: FGameSessionMetadata's members in offset order (they replicate property by property,
// no NetSerialize), the client class's only net field the RPC.

UENUM()
enum class EHypeGameMode : uint8
{
    Freestyle,
    Show,
    Record,
    Playback,
    MultiplayerFreestyle,
    MultiplayerBattle,
    MultiplayerMainStage,
    StageKitEditOnly,
    EditorRenderCutScene,
    Num,
};

UENUM()
enum class EVenueTimeOfDay : uint8
{
    Morning,
    Day,
    Sunset,
    Night,
    Num,
};

UENUM()
enum class ESessionBrowserEntrySource : uint8
{
    Unknown,
    PlatformFriend,
    Followship,
    Invite,
    Public,
};

USTRUCT()
struct HYPE_API FGameSessionMetadata
{
    GENERATED_BODY()

    UPROPERTY()
    FString gameSessionId;

    UPROPERTY()
    FString SessionName;

    UPROPERTY()
    bool IsPrivateSession = false;

    UPROPERTY()
    int32 LeaderHypeUserId = -1;

    UPROPERTY()
    FString LeaderUserName;

    UPROPERTY()
    int32 NumDJs = 0;

    UPROPERTY()
    int32 NumSpectators = 0;

    UPROPERTY()
    int32 FreestyleTurnLengthMeasures = -1;

    UPROPERTY()
    int32 FreestyleTotalRoundNumber = -1;

    UPROPERTY()
    EMultiplayerGameplayStage CurrentState = EMultiplayerGameplayStage::Lobby;

    UPROPERTY()
    UVenueData* venue = nullptr;

    UPROPERTY()
    EVenueTimeOfDay VenueTimeOfDay = EVenueTimeOfDay::Morning;

    UPROPERTY()
    EHypeGameMode GameMode = EHypeGameMode::MultiplayerFreestyle;

    UPROPERTY()
    FString IPAdress; // sic (FUSER's name)

    UPROPERTY()
    FString Port;

    UPROPERTY()
    float SessionDurationSeconds = 0.0f;

    UPROPERTY()
    ESessionBrowserEntrySource BrowserEntrySource = ESessionBrowserEntrySource::Unknown;
};

UCLASS()
class HYPE_API AHypeMultiplayerBeaconClient : public AOnlineBeaconClient
{
    GENERATED_BODY()

public:
    UFUNCTION(Client, Reliable)
    void ClientSendGameSessionMetadata(const FGameSessionMetadata& MetaData);
};

UCLASS()
class HYPE_API AHypeMultiplayerBeaconHost : public AOnlineBeaconHost
{
    GENERATED_BODY()
};

UCLASS()
class HYPE_API AHypeMultiplayerBeaconHostObject : public AOnlineBeaconHostObject
{
    GENERATED_BODY()

public:
    AHypeMultiplayerBeaconHostObject(const FObjectInitializer& ObjectInitializer);

    // FUN_140708b10: once a client's beacon has joined, answer with the session's metadata, which the server fills.
    virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;

    // Set by the server (FuserServer FDSBeacon): fills the metadata for one reply.
    static TFunction<void(FGameSessionMetadata&)> MetadataProvider;
};
