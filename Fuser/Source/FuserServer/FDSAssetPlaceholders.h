#pragma once

#include "CoreMinimal.h"

// Game content the dedicated server does not have (songs, instruments, effects, moments, venue data that clients
// reference in RPC parameters). Instead of loading the licensed assets, FDS keeps an empty object of the expected class
// under the same path; the server replicates the path back and every client resolves its own asset.
// Requires FDS's engine patch FNetGUIDCache::ServerMissingObjectResolver.
namespace FDSAssetPlaceholders
{
    // Installs the resolver: placeholder packages for /Game paths FDS does not ship; objects inside them are deferred.
    void Install();
    void Uninstall();

    // Forget a deferred reference (start of each object deserialization).
    void ClearDeferred();

    // Creates the placeholder for the reference the resolver just deferred, as an object of the expected class.
    // Returns null when nothing was deferred or the class is not placeholder content (actors, components, abstract).
    UObject* CreateDeferred(UClass* ExpectedClass);

    // A placeholder the server itself references, e.g. the venue in the session browser's metadata (FDSBeacon): the
    // object /Game/...Package.Name of the given class, created once; clients resolve the path to their own asset.
    UObject* GetOrCreate(const FString& PackageName, FName ObjectName, UClass* Class);
}
