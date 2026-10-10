#pragma once

#include "CoreMinimal.h"

// The session's venue and time of day.
//
// FUSER's venue catalogue is its VenueData assets (/Game/Gameplay/Venues/VenueA..F): MapFile (the persistent map,
// e.g. /Game/Maps/Venue01/Venue01) and LightingSublevels (time of day -> skybox sublevel). FDS reads those two fields
// from the FUSER install's packages without loading them: FUSER's cooked packages carry FAssetRegistryVersion 17,
// newer than stock UE4.24's 6, so the engine's loader refuses every one of them ("saved with a newer custom
// version"). The package summary, name and export tables are read with the engine's serializers; the property tags
// with a minimal reader for UE4.24's tag format.
//
// The server world is FDS's own empty stub of the venue's map (Config/FDSVenueMaps.txt, all six venues), never
// FUSER's map. Clients load their own copy of the venue and its sublevels; FDS only has to own (or, with the install
// mounted, see) every sublevel package a client reports visible.
//
// Selection: -FDSVenue= / FDS_VENUE / FUSER's server_venue=, as a venue ID (VenueC), a map name (Venue03) or a map
// package (/Game/Maps/Venue03/Venue03); -FDSTimeOfDay= / FDS_TIME_OF_DAY: morning, day, sunset or night. Without
// either, the configured ServerDefaultMap (Venue01) and no time of day.
namespace FDSVenues
{
    struct FVenue
    {
        FString Id;                     // VenueA
        FString Map;                    // /Game/Maps/Venue01/Venue01
        TMap<FString, FString> Lighting; // morning|day|sunset|night -> sublevel package
    };

    struct FSelection
    {
        FString Id;        // empty when the venue is not in the catalogue (no install)
        FString Map;       // the server world
        FString TimeOfDay; // empty when not given
    };

    // Reads the catalogue from the mounted install (FDSInstallContent) and applies the selection to the server's
    // default map. Call at module startup after FDSInstallContent::Mount, before the engine loads its first map.
    void Select();

    const TArray<FVenue>& Catalog();
    const FSelection& Selected();
}
