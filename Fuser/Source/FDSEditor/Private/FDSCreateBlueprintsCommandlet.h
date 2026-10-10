#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FDSCreateBlueprintsCommandlet.generated.h"

// Creates FDS's blueprint stubs under FUSER's asset paths: empty blueprints whose parents and network-relevant class
// defaults reproduce FUSER's cooked blueprints. No original content is copied.
//   UE4Editor-Cmd Fuser.uproject -run=FDSCreateBlueprints
UCLASS()
class UFDSCreateBlueprintsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UFDSCreateBlueprintsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
