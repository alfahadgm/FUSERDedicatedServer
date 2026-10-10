#pragma once

#include "CoreMinimal.h"

// A licensed FUSER install, mounted read-only into the container, as a source of
// FUSER's packages. Its paks (pakchunk*-WindowsNoEditor.pak, version 8, index and files AES-256 encrypted with FUSER's
// default key) are mounted below FDS's own pak, so FDS's config, stub worlds and Blueprints always win. FUSER's pak
// entries are rooted at Fuser/Content, the same as FDS's project, so their packages appear under /Game.
//
// What FDS may load from the install is an allowlist ([/Script/FuserServer.FDSInstall] +AllowLoad= package path
// prefixes). Everything else a client references keeps a placeholder (FDSAssetPlaceholders): the install's assets carry audio,
// Blueprints and native classes FDS doesn't have. Package existence (the level-visibility check for streamed
// sublevels) needs no load and works for every install package.
//
// Settings: FDS_FUSER_ROOT (environment) or -FDSFuserRoot= is the install's root (the folder holding Fuser/); unset,
// FDS runs without an install. FDS_PAK_KEY (environment only: UE4 logs its command line) is FUSER's AES key, 64 hex
// digits (optional 0x) or base64; it is never logged.
namespace FDSInstallContent
{
    enum class ESource : uint8
    {
        None,    // no such package
        Own,     // FDS's own content (its pak, or loose files in development)
        Install, // only in the FUSER install's paks
    };

    // Mounts the install if configured. Call once at module startup, before any content loads.
    void Mount();

    bool IsMounted();

    // Where a long package name (/Game/...) comes from, by the pak that serves its file.
    ESource SourceOf(const FString& LongPackageName);

    // Whether FDS may load this install package (the allowlist). Logs each refused package once.
    bool IsLoadAllowed(const FString& LongPackageName);
}
