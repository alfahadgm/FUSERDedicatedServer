#pragma once

#include "CoreMinimal.h"

// Admission with fuserd-signed session tokens. fuserd issues the player session id a client joins with as a token:
//   psess-fds1.<base64url(claims JSON)>.<base64url(Ed25519 signature over "fds1.<base64url(claims JSON)>")>
// claims: {"v":1, "slot", "gsid", "psid" (random), "uid" (Hype user id), "exp" (Unix seconds)}.
// FDS holds only fuserd's public key (FDS_TOKEN_PUBLIC_KEY, 32 bytes base64url). UE4.24 has no Ed25519 or SHA-512, so
// the verifier here is a port of TweetNaCl's crypto_sign_open (public domain), checked against Go's crypto/ed25519
// by the FDSTokenSelfTest commandlet.
namespace FDSSessionToken
{
    struct FClaims
    {
        FString Slot;
        FString GameSession;
        FString PlayerSession;
        int64 User = 0;
        int64 Expires = 0;
    };

    // fuserd's public key; false if it isn't 32 bytes of base64url.
    bool SetPublicKey(const FString& Base64Url);
    bool HasPublicKey();

    // Signature, format and expiry. Returns an empty string on success, else why the token was refused (no secrets).
    FString Verify(const FString& Token, int64 NowUnixSeconds, FClaims& OutClaims);

    // Ed25519 verification of a raw signature (64 bytes) over a message with a 32-byte public key.
    bool VerifyEd25519(const uint8* PublicKey, const uint8* Message, int32 MessageLength, const uint8* Signature);

    // Base64url (no padding) decoding; false on a bad character.
    bool DecodeBase64Url(const FString& Text, TArray<uint8>& Out);
}
