#include "FDSAssetPlaceholders.h"
#include "FDSInstallContent.h"
#include "FuserServer.h"
#include "Components/ActorComponent.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    // Placeholders live for the server's lifetime (a bounded set: the content the players' crates reference).
    TSet<UPackage*>& PlaceholderPackages()
    {
        static TSet<UPackage*> Set;
        return Set;
    }

    // The reference the resolver deferred during the current object deserialization (game thread only).
    struct FDeferred
    {
        FName Name;
        UPackage* Package = nullptr;
    };
    FDeferred GDeferred;

    FNetGUIDCache::EServerMissingObject Resolve(FName PathName, UObject* Outer, bool bIsPackage, UObject*& OutObject)
    {
        using E = FNetGUIDCache::EServerMissingObject;
        if (bIsPackage)
        {
            const FString Path = PathName.ToString();
            // Only game content: FDS's own maps and blueprint stubs load normally, and so do install packages on the
            // load allowlist (FDSInstallContent). Anything else - not present, or only in the FUSER install - gets a placeholder.
            if (!Path.StartsWith(TEXT("/Game/")))
            {
                return E::Declined;
            }
            const FDSInstallContent::ESource Source = FDSInstallContent::SourceOf(Path);
            if (Source == FDSInstallContent::ESource::Own || (Source == FDSInstallContent::ESource::Install && FDSInstallContent::IsLoadAllowed(Path)))
            {
                return E::Declined;
            }
            UPackage* Package = CreatePackage(nullptr, *Path);
            Package->SetFlags(RF_Public | RF_WasLoaded);
            Package->MarkAsFullyLoaded();
            Package->AddToRoot();
            PlaceholderPackages().Add(Package);
            OutObject = Package;
            return E::Created;
        }
        // An object inside a placeholder package: its class is known only to the caller deserializing the reference.
        UPackage* OuterPackage = Cast<UPackage>(Outer);
        if (OuterPackage && PlaceholderPackages().Contains(OuterPackage))
        {
            GDeferred.Name = PathName;
            GDeferred.Package = OuterPackage;
            return E::Deferred;
        }
        return E::Declined;
    }
}

void FDSAssetPlaceholders::Install()
{
    FNetGUIDCache::ServerMissingObjectResolver = &Resolve;
}

void FDSAssetPlaceholders::Uninstall()
{
    FNetGUIDCache::ServerMissingObjectResolver = nullptr;
}

void FDSAssetPlaceholders::ClearDeferred()
{
    GDeferred = FDeferred();
}

UObject* FDSAssetPlaceholders::GetOrCreate(const FString& PackageName, FName ObjectName, UClass* Class)
{
    if (!PackageName.StartsWith(TEXT("/Game/")) || ObjectName.IsNone() || !Class)
    {
        return nullptr;
    }
    UPackage* Package = FindPackage(nullptr, *PackageName);
    if (!Package)
    {
        Package = CreatePackage(nullptr, *PackageName);
        Package->SetFlags(RF_Public | RF_WasLoaded);
        Package->MarkAsFullyLoaded();
        Package->AddToRoot();
        PlaceholderPackages().Add(Package);
    }
    UObject* Object = StaticFindObjectFast(nullptr, Package, ObjectName);
    if (!Object)
    {
        Object = NewObject<UObject>(Package, Class, ObjectName, RF_Public | RF_Standalone | RF_WasLoaded | RF_LoadCompleted);
        Object->AddToRoot();
        UE_LOG(LogFDS, Display, TEXT("FDS: placeholder %s for %s.%s (server reference)"), *Class->GetName(), *PackageName, *ObjectName.ToString());
    }
    return Object->IsA(Class) ? Object : nullptr;
}

UObject* FDSAssetPlaceholders::CreateDeferred(UClass* ExpectedClass)
{
    const FDeferred Deferred = GDeferred;
    GDeferred = FDeferred();
    if (!Deferred.Package || !ExpectedClass)
    {
        return nullptr;
    }
    // Data only: never actors, components or abstract classes.
    if (ExpectedClass->HasAnyClassFlags(CLASS_Abstract) || ExpectedClass->IsChildOf(AActor::StaticClass()) ||
        ExpectedClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: no placeholder for %s.%s: class %s is not placeholder content"), *Deferred.Package->GetName(),
            *Deferred.Name.ToString(), *ExpectedClass->GetName());
        return nullptr;
    }
    UObject* Object = StaticFindObjectFast(nullptr, Deferred.Package, Deferred.Name);
    if (!Object)
    {
        // RF_WasLoaded makes the name stable for networking, so the server can export the same path to other clients.
        Object = NewObject<UObject>(Deferred.Package, ExpectedClass, Deferred.Name, RF_Public | RF_Standalone | RF_WasLoaded | RF_LoadCompleted);
        Object->AddToRoot();
        UE_LOG(LogFDS, Display, TEXT("FDS: placeholder %s for %s.%s"), *ExpectedClass->GetName(), *Deferred.Package->GetName(), *Deferred.Name.ToString());
    }
    if (!Object->IsA(ExpectedClass))
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: placeholder %s is a %s, now referenced as %s"), *Object->GetPathName(), *Object->GetClass()->GetName(),
            *ExpectedClass->GetName());
        return nullptr;
    }
    return Object;
}
