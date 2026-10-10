#pragma once

#include "CoreMinimal.h"

class UWorld;
class UFDSLobbyAuthority;

// FUSER's session-browser beacon. The client lists a session from fuserd's get_list
// only when the session's beacon (game port + 100) answers with its FGameSessionMetadata; FDS runs the Hype beacon
// host there and fills the reply as FUSER's FUN_140708b10 did:
//   gameSessionId           fuserd's session id (from the registration replies; empty before a ticket completed)
//   SessionName             empty (as FUSER)
//   IsPrivateSession        the lobby's access (FDS_SERVER_ACCESS)
//   LeaderHypeUserId / Name the session leader's
//   NumDJs / NumSpectators  the DJ team / everyone else
//   turn length / rounds    the set length in use
//   CurrentState            the current stage
//   venue                   a placeholder VenueData at the selected venue's path (clients resolve their own asset)
//   VenueTimeOfDay          the selected time of day
//   GameMode                MultiplayerFreestyle
//   IPAdress / Port         FDS_ADDRESS (fuserd's advertised address) and the game port
//   SessionDurationSeconds  since the server started
namespace FDSBeacon
{
    void Start(UWorld* World, UFDSLobbyAuthority* Lobby);
    // The UDP port the beacon is bound to (the net driver may have moved past a taken port), 0 when it is not
    // listening. Reported to fuserd in the registration (FDSRegistration).
    int32 BoundPort();
}
