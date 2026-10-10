#pragma once

#include "CoreMinimal.h"

class AController;
class APlayerController;

// Who may join. A container fuserd starts gets fuserd's public key
// (FDS_TOKEN_PUBLIC_KEY) and then requires every client to log in with a fuserd-issued session token as its
// playerSessionId (FDSSessionToken). The token must:
//   - carry a valid signature and not be expired (expiry bounds the first join: a token this server has admitted
//     stays good here for a reconnect);
//   - name this container's slot (FDS_SLOT_ID);
//   - match the client's own hypeUserId and gameSessionId login options, when present;
//   - not already be in use by a connected player (one live connection per token; a reconnect after the old
//     connection is gone is allowed).
// Without a key (local and direct-join runs) any client is admitted, as before. FDS_REQUIRE_TOKEN=0 turns the check
// off even with a key; FDS_REQUIRE_TOKEN=1 without a usable key refuses everyone.
namespace FDSAdmission
{
    void Init();
    bool IsRequired();

    // PreLogin: empty when the client may join, else the reason (logged by the caller; never the token itself).
    FString Check(const FString& Options);

    // InitNewPlayer / Logout: track the token's player session against the connection holding it.
    void Admitted(const FString& Options, APlayerController* Player);
    void Left(AController* Player);
}
