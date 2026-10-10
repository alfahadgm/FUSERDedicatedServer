#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FDSCommandlets.generated.h"

// Editor commandlets (UE4Editor-Cmd Fuser.uproject -run=<name>).

// Creates FDS's own empty map packages under the package names FUSER clients travel to (no original content).
//   -run=FDSCreateMaps [-maps=/Game/Maps/Server/ServerDefaultMap+...]
UCLASS()
class UFDSCreateMapsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UFDSCreateMapsCommandlet();
    virtual int32 Main(const FString& Params) override;
};

// Writes FDS's replication layout as JSON (classes / structs / class net checksums), for comparison with the layout
// recorded from a FUSER client.
//   -run=FDSNetLayout -classes=<file of class paths, one per line> -out=<json>
UCLASS()
class UFDSNetLayoutCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UFDSNetLayoutCommandlet();
    virtual int32 Main(const FString& Params) override;
};

// Checks FDS's Ed25519 session-token verifier (FDSSessionToken) against vectors from Go's crypto/ed25519: RFC 8032's
// first test, good and corrupted signatures, and fuserd-issued tokens with the right and a wrong key. Exit code 0
// when every vector behaves.
//   -run=FDSTokenSelfTest -vectors=<vectors JSON file>
UCLASS()
class UFDSTokenSelfTestCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UFDSTokenSelfTestCommandlet();
    virtual int32 Main(const FString& Params) override;
};
