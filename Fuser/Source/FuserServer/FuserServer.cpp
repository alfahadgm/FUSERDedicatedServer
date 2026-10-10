#include "FuserServer.h"
#include "FDSAssetPlaceholders.h"
#include "FDSInstallContent.h"
#include "FDSVenues.h"
#include "HypeMultiplayerBeacon.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/NetworkVersion.h"
#include "Misc/Parse.h"
#include "Misc/CoreDelegates.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(LogFDS);

namespace
{
    // FUSER binds a game delegate (FUN_1406906b0 -> GameInstance+0x380) that the Hello check compares exactly; the
    // EGS v1.4.2 client reports 10402. -FDSNetVersion=<n> overrides it, e.g. for rejection tests.
    constexpr uint32 FuserNetworkVersion = 10402;
    uint32 GNetworkVersion = FuserNetworkVersion;
}

class FFuserServerModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        uint32 Override = 0;
        if (FParse::Value(FCommandLine::Get(), TEXT("FDSNetVersion="), Override))
        {
            GNetworkVersion = Override;
        }
        FNetworkVersion::GetLocalNetworkVersionOverride.BindLambda([]() { return GNetworkVersion; });
        FDSInstallContent::Mount(); // before any content loads
        FDSVenues::Select();        // the venue catalogue from the install; the server world
        FDSAssetPlaceholders::Install();
        FNetworkVersion::InvalidateNetworkChecksum();
        UE_LOG(LogFDS, Display, TEXT("FDS: network version %u (FUSER EGS v1.4.2 value %u)%s"), GNetworkVersion, FuserNetworkVersion,
            GNetworkVersion == FuserNetworkVersion ? TEXT("") : TEXT(" - command-line override"));

        // Development builds include the gameplay debugger, whose per-world manager spawns a replicated
        // GameplayDebuggerCategoryReplicator for every player. FUSER's shipping clients lack that class and answer
        // ActorChannelFailure. Disabling it at build level would rebuild the engine, so its manager is
        // stopped and any replicator removed as soon as they appear. Matched by class name: no module dependency.
        FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
        {
            if (!World || !World->IsGameWorld())
            {
                return;
            }
            auto Suppress = [](AActor* Actor)
            {
                const FName ClassName = Actor ? Actor->GetClass()->GetFName() : NAME_None;
                if (ClassName == TEXT("GameplayDebuggerPlayerManager") || ClassName == TEXT("GameplayDebuggerCategoryReplicator"))
                {
                    Actor->SetActorTickEnabled(false);
                    Actor->Destroy();
                    UE_LOG(LogFDS, Display, TEXT("FDS: removed %s (development-build gameplay debugger)"), *ClassName.ToString());
                }
            };
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                Suppress(*It);
            }
            World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda(Suppress));
        });

        // Test hook: -FDSBeaconProbe=<host:port> makes a -game client query a server's
        // session-browser beacon as FUSER's browser does, and log the metadata (HypeMultiplayerBeacon.cpp).
        FWorldDelegates::OnWorldInitializedActors.AddLambda([](const UWorld::FActorsInitializedParams& Params)
        {
            static bool bProbed = false;
            FString Probe;
            if (bProbed || !Params.World || !Params.World->IsGameWorld() || Params.World->GetNetMode() == NM_DedicatedServer ||
                !FParse::Value(FCommandLine::Get(), TEXT("FDSBeaconProbe="), Probe))
            {
                return;
            }
            bProbed = true;
            AHypeMultiplayerBeaconClient* Client = Params.World->SpawnActor<AHypeMultiplayerBeaconClient>();
            FURL Url(nullptr, *Probe, TRAVEL_Absolute);
            const bool bStarted = Client && Client->InitClient(Url);
            UE_LOG(LogFDS, Display, TEXT("FDS beacon probe: querying %s:%d: %s"), *Url.Host, Url.Port, bStarted ? TEXT("connecting") : TEXT("FAILED"));
        });

        FCoreDelegates::OnPostEngineInit.AddLambda([]()
        {
            if (!GEngine)
            {
                return;
            }
            GEngine->OnNetworkFailure().AddLambda([](UWorld*, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Message)
            {
                UE_LOG(LogFDS, Warning, TEXT("FDS: network failure %s on %s: %s"), ENetworkFailure::ToString(Type),
                    Driver ? *Driver->NetDriverName.ToString() : TEXT("?"), *Message);
            });
            GEngine->OnTravelFailure().AddLambda([](UWorld*, ETravelFailure::Type Type, const FString& Message)
            {
                UE_LOG(LogFDS, Warning, TEXT("FDS: travel failure %s: %s"), ETravelFailure::ToString(Type), *Message);
            });
        });
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFuserServerModule, FuserServer, "FuserServer");
