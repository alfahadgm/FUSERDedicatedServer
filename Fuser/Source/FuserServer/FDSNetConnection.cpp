#include "FDSNetConnection.h"
#include "FDSAssetPlaceholders.h"

bool UFDSPackageMapClient::SerializeObject(FArchive& Ar, UClass* InClass, UObject*& Obj, FNetworkGUID* OutNetGUID)
{
    if (!Ar.IsLoading() || !IsNetGUIDAuthority())
    {
        return Super::SerializeObject(Ar, InClass, Obj, OutNetGUID);
    }
    FDSAssetPlaceholders::ClearDeferred();
    FNetworkGUID NetGUID;
    const bool bMapped = Super::SerializeObject(Ar, InClass, Obj, &NetGUID);
    if (!Obj && NetGUID.IsDefault())
    {
        // The client referenced content the server does not have (resolver deferred it): stand in a placeholder of the
        // expected class, then assign and announce its NetGUID as the stock code does for a resolved default GUID.
        if (UObject* Placeholder = FDSAssetPlaceholders::CreateDeferred(InClass))
        {
            TrackedUnmappedNetGuids.Remove(NetGUID);
            Obj = Placeholder;
            NetGUID = GuidCache->GetOrAssignNetGUID(Placeholder);
            HandleUnAssignedObject(Placeholder);
            if (OutNetGUID)
            {
                *OutNetGUID = NetGUID;
            }
            return true;
        }
    }
    if (OutNetGUID)
    {
        *OutNetGUID = NetGUID;
    }
    return bMapped;
}

UFDSIpConnection::UFDSIpConnection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PackageMapClass = UFDSPackageMapClient::StaticClass();
}
