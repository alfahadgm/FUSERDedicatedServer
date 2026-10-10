#include "FDSCreateBlueprintsCommandlet.h"
#include "AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogFDSEditor, Log, All);

namespace
{
    struct FDefault
    {
        const TCHAR* Key;   // property on the generated class's CDO
        const TCHAR* Value; // ImportText value
    };

    struct FStub
    {
        const TCHAR* Package;
        const TCHAR* Parent;
        TArray<FDefault> Defaults;
    };

    // Parents and CDO values read from FUSER's cooked blueprints.
    // Dependencies first: the game mode's defaults name the other blueprint classes.
    TArray<FStub> Stubs()
    {
        return {
            {TEXT("/Game/GameFramework/BP_HypePlayerController"), TEXT("/Script/Hype.HypePlayerController"), {}},
            {TEXT("/Game/GameFramework/BP_HypePawn"), TEXT("/Script/Hype.ShellPawn"),
                {{TEXT("bOnlyRelevantToOwner"), TEXT("True")}, {TEXT("bReplicateMovement"), TEXT("False")}}},
            {TEXT("/Game/GameFramework/BP_HypeHUD"), TEXT("/Script/Hype.HypeHUD"), {}},
            // FUSER's parent is VenueGameMode; FDS's server behaviour lives in its subclass FDSVenueGameMode (no
            // replicated members, so the class path is all clients see). GameStateClass stays the parent's runtime
            // choice (HypeGameStateMultiplayerFreestyle) instead of FUSER's CDO HypeGameState.
            {TEXT("/Game/GameFramework/BP_VenueGameMode"), TEXT("/Script/FuserServer.FDSVenueGameMode"),
                {{TEXT("PlayerControllerClass"), TEXT("/Game/GameFramework/BP_HypePlayerController.BP_HypePlayerController_C")},
                 {TEXT("PlayerStateClass"), TEXT("/Script/Hype.HypePlayerState")},
                 {TEXT("HUDClass"), TEXT("/Game/GameFramework/BP_HypeHUD.BP_HypeHUD_C")},
                 {TEXT("DefaultPawnClass"), TEXT("/Game/GameFramework/BP_HypePawn.BP_HypePawn_C")}}},
        };
    }
}

UFDSCreateBlueprintsCommandlet::UFDSCreateBlueprintsCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UFDSCreateBlueprintsCommandlet::Main(const FString& Params)
{
    int32 Failures = 0;
    for (const FStub& Stub : Stubs())
    {
        UClass* Parent = LoadObject<UClass>(nullptr, Stub.Parent);
        if (!Parent)
        {
            UE_LOG(LogFDSEditor, Error, TEXT("FDSCreateBlueprints: parent %s not found"), Stub.Parent);
            ++Failures;
            continue;
        }
        const FString Name = FPackageName::GetShortName(Stub.Package);
        UPackage* Package = CreatePackage(nullptr, Stub.Package);
        if (UBlueprint* Existing = FindObject<UBlueprint>(Package, *Name))
        {
            Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
        }
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(*Name), BPTYPE_Normal,
            UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName(TEXT("FDSCreateBlueprints")));
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
        for (const FDefault& Default : Stub.Defaults)
        {
            UProperty* Property = FindField<UProperty>(Blueprint->GeneratedClass, Default.Key);
            const TCHAR* Result = Property ? Property->ImportText(Default.Value, Property->ContainerPtrToValuePtr<void>(CDO), PPF_None, CDO) : nullptr;
            if (!Result)
            {
                UE_LOG(LogFDSEditor, Error, TEXT("FDSCreateBlueprints: %s.%s = %s failed"), *Name, Default.Key, Default.Value);
                ++Failures;
            }
        }
        FAssetRegistryModule::AssetCreated(Blueprint);
        Package->MarkPackageDirty();
        const FString File = FPackageName::LongPackageNameToFilename(Stub.Package, FPackageName::GetAssetPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, Blueprint, RF_Public | RF_Standalone, *File, GError, nullptr, false, true, SAVE_NoError);
        UE_LOG(LogFDSEditor, Display, TEXT("FDSCreateBlueprints: %s (parent %s, class %s, %d defaults) -> %s"), Stub.Package, Stub.Parent,
            *Blueprint->GeneratedClass->GetPathName(), Stub.Defaults.Num(), bSaved ? TEXT("saved") : TEXT("FAILED"));
        Failures += bSaved ? 0 : 1;
    }
    return Failures;
}
