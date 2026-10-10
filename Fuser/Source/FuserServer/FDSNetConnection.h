#pragma once

#include "CoreMinimal.h"
#include "IpConnection.h"
#include "Engine/PackageMapClient.h"
#include "FDSNetConnection.generated.h"

// The package map FDS connections use: stock behaviour, plus content placeholders for references to game assets the
// dedicated server does not have (FDSAssetPlaceholders).
UCLASS(transient)
class FUSERSERVER_API UFDSPackageMapClient : public UPackageMapClient
{
    GENERATED_BODY()

public:
    virtual bool SerializeObject(FArchive& Ar, UClass* InClass, UObject*& Obj, FNetworkGUID* OutNetGUID = nullptr) override;
};

// IpConnection with the FDS package map (IpNetDriver NetConnectionClassName in DefaultEngine.ini).
UCLASS(transient, config = Engine)
class FUSERSERVER_API UFDSIpConnection : public UIpConnection
{
    GENERATED_BODY()

public:
    UFDSIpConnection(const FObjectInitializer& ObjectInitializer);
};
