#include "HypeMultiplayerBeacon.h"
#include "VenueData.h"

TFunction<void(FGameSessionMetadata&)> AHypeMultiplayerBeaconHostObject::MetadataProvider;

// Runs on the client (FUSER's session browser); the server only sends it. In FDS's own test client
// (-FDSBeaconProbe) it reports what arrived.
void AHypeMultiplayerBeaconClient::ClientSendGameSessionMetadata_Implementation(const FGameSessionMetadata& MetaData)
{
    UE_LOG(LogNet, Display, TEXT("FDS beacon probe: metadata received: session '%s', %s, leader %d '%s', %d DJ(s) %d viewer(s), %d x %d, stage %d, venue %s, time of day %d, mode %d, address '%s' port '%s', %.0f s"),
        *MetaData.gameSessionId, MetaData.IsPrivateSession ? TEXT("private") : TEXT("public"), MetaData.LeaderHypeUserId, *MetaData.LeaderUserName,
        MetaData.NumDJs, MetaData.NumSpectators, MetaData.FreestyleTotalRoundNumber, MetaData.FreestyleTurnLengthMeasures,
        static_cast<int32>(MetaData.CurrentState), MetaData.venue ? *MetaData.venue->GetPathName() : TEXT("null"),
        static_cast<int32>(MetaData.VenueTimeOfDay), static_cast<int32>(MetaData.GameMode), *MetaData.IPAdress, *MetaData.Port,
        MetaData.SessionDurationSeconds);
}

// FUSER's CDO (Default__HypeMultiplayerBeaconHostObject): BeaconTypeName HypeMultiplayerBeaconClient, the client
// actor class HypeMultiplayerBeaconClient.
AHypeMultiplayerBeaconHostObject::AHypeMultiplayerBeaconHostObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    BeaconTypeName = TEXT("HypeMultiplayerBeaconClient");
    ClientBeaconActorClass = AHypeMultiplayerBeaconClient::StaticClass();
}

void AHypeMultiplayerBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
    Super::OnClientConnected(NewClientActor, ClientConnection);
    if (AHypeMultiplayerBeaconClient* Client = Cast<AHypeMultiplayerBeaconClient>(NewClientActor))
    {
        FGameSessionMetadata MetaData;
        if (MetadataProvider)
        {
            MetadataProvider(MetaData);
        }
        Client->ClientSendGameSessionMetadata(MetaData);
    }
}
