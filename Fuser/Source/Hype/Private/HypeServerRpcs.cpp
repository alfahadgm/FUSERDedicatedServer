// Hand-written server RPC bodies (excluded from generation). Each follows the FUSER EGS v1.4.2 native implementation
// named beside it; server decisions are forwarded to the registered IHypeServerAuthority.
#include "HypeServerAuthority.h"
#include "DJPlayerState.h"
#include "HypePlayerController.h"
#include "HypePlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeServer, Log, All);

namespace
{
    TMap<TWeakObjectPtr<UWorld>, IHypeServerAuthority*>& Authorities()
    {
        static TMap<TWeakObjectPtr<UWorld>, IHypeServerAuthority*> Map;
        return Map;
    }
}

void IHypeServerAuthority::Register(UWorld* World, IHypeServerAuthority* Authority)
{
    Authorities().Add(World, Authority);
}

void IHypeServerAuthority::Unregister(UWorld* World, IHypeServerAuthority* Authority)
{
    if (Authorities().FindRef(World) == Authority)
    {
        Authorities().Remove(World);
    }
}

IHypeServerAuthority* IHypeServerAuthority::Get(const UObject* WorldContext)
{
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    return World ? Authorities().FindRef(World) : nullptr;
}

// ---------------------------------------------------------------------------------------------- AHypePlayerState

// FUN_140696260: store the id, then the game server picks the team (FUN_140705120) and FUN_1406963e0 sets it.
void AHypePlayerState::ServerSetHypeUserId_Implementation(int32 hypeUserId)
{
    _hypeUserId = hypeUserId;
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSetHypeUserId(this);
    }
}
// The native validator refuses -1 (the "no user" id): the sender is disconnected, as by FUSER's own server.
bool AHypePlayerState::ServerSetHypeUserId_Validate(int32 hypeUserId) { return hypeUserId != -1; }

// FUN_140696300: in Lobby (or Battle_WaitForPlayers) set/clear the Lobby ready bit.
void AHypePlayerState::ServerSetIsReadyInLobby_Implementation(bool IsReady)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSetIsReadyInLobby(this, IsReady);
    }
}
bool AHypePlayerState::ServerSetIsReadyInLobby_Validate(bool IsReady) { return true; }

// FUN_140696370: in Results_XPPayout set/clear the Results ready bit.
void AHypePlayerState::ServerSetIsReadyInResults_Implementation(bool IsReady)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSetIsReadyInResults(this, IsReady);
    }
}
bool AHypePlayerState::ServerSetIsReadyInResults_Validate(bool IsReady) { return true; }

// FUN_140695d20: forwarded to the team-change rule (FUN_14062bee0) when the team differs.
void AHypePlayerState::ServerRequestMultiplayerTeamChange_Implementation(EMultiplayerTeam newTeam)
{
    if (newTeam != EMultiplayerTeam::Unknown && newTeam != _team)
    {
        if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
        {
            Authority->OnRequestTeamChange(this, newTeam);
        }
    }
}
bool AHypePlayerState::ServerRequestMultiplayerTeamChange_Validate(EMultiplayerTeam newTeam) { return true; }

// FUN_140695c40: only the session leader may complete the lobby (FUN_14062bac0).
void AHypePlayerState::ServerMarkLobbyComplete_Implementation()
{
    if (_isSessionLeader)
    {
        if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
        {
            Authority->OnMarkLobbyComplete(this);
        }
    }
}
bool AHypePlayerState::ServerMarkLobbyComplete_Validate() { return true; }

// FUN_140695940: the leader hands leadership to another player (FUN_1406965a0).
void AHypePlayerState::ServerChangeLeaderTo_Implementation(AHypePlayerState* ps)
{
    if (ps && _isSessionLeader)
    {
        if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
        {
            Authority->OnChangeLeaderTo(this, ps);
        }
    }
}
bool AHypePlayerState::ServerChangeLeaderTo_Validate(AHypePlayerState* ps) { return true; }

// FUN_140696010: in the Lobby stage (FUN_1406243c0), the HypeGameState multicasts the move (FUN_140829470).
void AHypePlayerState::ServerSendSignatureMove_Implementation()
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSendSignatureMove(this);
    }
}
bool AHypePlayerState::ServerSendSignatureMove_Validate() { return true; }

// ---------------------------------------------------------------------------------------------- DJ actions
// The DJ action RPCs are empty in FUSER's shipping build (shared stub FUN_14033dfb0): the dedicated server
// appended each one to the sender's _multiplayerActions, which every other client's consumer (FUN_14069bc90) replays
// when its own clock reaches MasterTimelineTick. RPC -> action type and fields follow the item's native serializer
// (FUN_140626eb0).
namespace
{
    FMultiplayerActionItemEntry MakeAction(EMultiplayerActionType Type, int32 Tick)
    {
        FMultiplayerActionItemEntry Item;
        Item.ActionType = Type;
        Item.MasterTimelineTick = Tick;
        return Item;
    }

    void RecordAction(AHypePlayerState* Player, FMultiplayerActionItemEntry Item)
    {
        if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(Player))
        {
            Authority->OnRecordAction(Player, Item);
        }
    }
}

void AHypePlayerState::ServerSpawnDisc_Implementation(int32 Tick, FName celShortName, ETrackType trackType, const FTransform& spawnTransform,
    EActionAfterDiscSpawn actionAfterDiscSpawn, int32 dropStartTick, int32 dropEndTick, bool isBattleSuperDrop)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::SpawnDisc, Tick);
    Item.celShortName = celShortName;
    Item.trackType = trackType;
    Item.spawnTransform = spawnTransform;
    Item.actionAfterDiscSpawn = actionAfterDiscSpawn;
    Item.dropStartTick = dropStartTick;
    Item.dropEndTick = dropEndTick;
    Item.isBattleSuperDrop = isBattleSuperDrop;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerSpawnDisc_Validate(int32 Tick, FName celShortName, ETrackType trackType, const FTransform& spawnTransform,
    EActionAfterDiscSpawn actionAfterDiscSpawn, int32 dropStartTick, int32 dropEndTick, bool isBattleSuperDrop) { return true; }

void AHypePlayerState::ServerEjectTrack_Implementation(int32 Tick, ETrackType trackType)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::EjectDisc, Tick);
    Item.trackType = trackType;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerEjectTrack_Validate(int32 Tick, ETrackType trackType) { return true; }

void AHypePlayerState::ServerHotSwapOnDeck_Implementation(int32 Tick, ETrackType trackType)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::HotSwapOnDeck, Tick);
    Item.trackType = trackType;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerHotSwapOnDeck_Validate(int32 Tick, ETrackType trackType) { return true; }

void AHypePlayerState::ServerDoDropQueue_Implementation(int32 Tick)
{
    RecordAction(this, MakeAction(EMultiplayerActionType::DoDropQueue, Tick));
}
bool AHypePlayerState::ServerDoDropQueue_Validate(int32 Tick) { return true; }

void AHypePlayerState::ServerEjectQueue_Implementation(int32 Tick, ETrackType trackType)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::EjectQueue, Tick);
    Item.trackType = trackType;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerEjectQueue_Validate(int32 Tick, ETrackType trackType) { return true; }

void AHypePlayerState::ServerUpdateAudioEffectState_Implementation(int32 Tick, EAudioEffectSlot slot, const FAudioEffectInfo& Info)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::EffectSlotState, Tick);
    Item.slot = slot;
    Item.SlotInfo = Info;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerUpdateAudioEffectState_Validate(int32 Tick, EAudioEffectSlot slot, const FAudioEffectInfo& Info) { return true; }

void AHypePlayerState::ServerUpdateSoloMuteState_Implementation(int32 Tick, const TArray<FDeckMuteSoloState>& soloMuteDeckStates)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::SoloMuteStates, Tick);
    Item.DeckStates = soloMuteDeckStates;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerUpdateSoloMuteState_Validate(int32 Tick, const TArray<FDeckMuteSoloState>& soloMuteDeckStates) { return true; }

void AHypePlayerState::ServerSetGainForTrack_Implementation(int32 Tick, ETrackType trackType, float gain)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::TrackGain, Tick);
    Item.trackType = trackType;
    Item.gain = gain;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerSetGainForTrack_Validate(int32 Tick, ETrackType trackType, float gain) { return true; }

void AHypePlayerState::ServerBeginInstrumentCrafting_Implementation(int32 Tick, EInstrumentCraftingSlot slot, UInstrumentCelData* InstrumentCelData)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::CurrentCraftedInstrument, Tick);
    Item.InstSlot = slot;
    Item.InstrumentCelData = InstrumentCelData;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerBeginInstrumentCrafting_Validate(int32 Tick, EInstrumentCraftingSlot slot, UInstrumentCelData* InstrumentCelData) { return true; }

void AHypePlayerState::ServerEndInstrumentCrafting_Implementation(int32 Tick, EInstrumentCraftingSlot slot)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::CurrentCraftedInstrument, Tick);
    Item.InstSlot = slot;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerEndInstrumentCrafting_Validate(int32 Tick, EInstrumentCraftingSlot slot) { return true; }

void AHypePlayerState::ServerUpdateRequestCompletion_Implementation(int32 Tick, int32 uniqueRequestId, EMultiplayerLiveRequestCompletionType completionType)
{
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::RequestComplete, Tick);
    Item.uniqueRequestId = uniqueRequestId;
    Item.completionType = completionType;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerUpdateRequestCompletion_Validate(int32 Tick, int32 uniqueRequestId, EMultiplayerLiveRequestCompletionType completionType) { return true; }

// Effect and instrument parameters (also empty in shipping): a time series per slot in the sender's unreliable arrays.
// The current DJ's listeners (FUN_14069b8d0) take, for each FX slot 3-7 / instrument slot 0-3, the last update before
// their own clock and the next one after it, and interpolate (FUN_14061d740 / FUN_14061d970).
void AHypePlayerState::ServerUpdateAudioEffectParams_Implementation(int32 Tick, EAudioEffectSlot slot, float paramX, float paramY)
{
    FMultiplayerUnreliableActionItemEntry Item;
    Item.ActionType = EMultiplayerUnreliableActionType::FXParam;
    Item.MasterTimelineTick = Tick;
    Item.FXSlot = slot;
    Item.paramX = paramX;
    Item.paramY = paramY;
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnRecordParam(this, Item);
    }
}
bool AHypePlayerState::ServerUpdateAudioEffectParams_Validate(int32 Tick, EAudioEffectSlot slot, float paramX, float paramY) { return true; }

void AHypePlayerState::ServerUpdateInstrumentCraftingState_Implementation(int32 Tick, EInstrumentCraftingSlot slot, const FGridInstrumentCraftingState& newState)
{
    FMultiplayerUnreliableActionItemEntry Item;
    Item.ActionType = EMultiplayerUnreliableActionType::InstrumentParam;
    Item.MasterTimelineTick = Tick;
    Item.InstSlot = slot;
    Item.InstState = newState;
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnRecordParam(this, Item);
    }
}
bool AHypePlayerState::ServerUpdateInstrumentCraftingState_Validate(int32 Tick, EInstrumentCraftingSlot slot, const FGridInstrumentCraftingState& newState) { return true; }

// FUN_140695870 (shipping): the instrument goes into the GameState's crafted registry (FUN_1406922d0, keyed by
// shortName) and to every other player through ClientSendCraftedInstrumentCel.
void AHypePlayerState::ServerAddCraftedInstrumentCel_Implementation(const FSerializableCraftedInstrumentCel& Data)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnAddCraftedInstrument(this, Data);
    }
}
bool AHypePlayerState::ServerAddCraftedInstrumentCel_Validate(const FSerializableCraftedInstrumentCel& Data) { return true; }

// FUN_140695cb0 (shipping): the name joins this player's pending requests (PS +0x1100) and FUN_14069f770 answers
// every pending name the registry holds. A listener sends it when a SpawnDisc's cel is unknown (FUN_14062bcd0).
void AHypePlayerState::ServerRequestCraftedInstrumentCel_Implementation(FName instrumentCelShortName)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnRequestCraftedInstrument(this, instrumentCelShortName);
    }
}
bool AHypePlayerState::ServerRequestCraftedInstrumentCel_Validate(FName instrumentCelShortName) { return true; }

// FUN_140695d60 (shipping): freestyle multicasts the praises to everyone, then counts them into the praised DJs' results.
// The unreliable variant's body (FUN_140695ff0) calls the same implementation.
void AHypePlayerState::ServerSendPraises_Implementation(const TArray<FCollectedPraise>& praises)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSendPraises(this, praises);
    }
}
bool AHypePlayerState::ServerSendPraises_Validate(const TArray<FCollectedPraise>& praises) { return true; }
void AHypePlayerState::ServerSendPraises_Unreliable_Implementation(const TArray<FCollectedPraise>& praises)
{
    ServerSendPraises_Implementation(praises);
}
bool AHypePlayerState::ServerSendPraises_Unreliable_Validate(const TArray<FCollectedPraise>& praises) { return true; }

// FUN_140696080 (shipping): a snapshot counts into toPlayer's results at that measure (FUN_140651760).
void AHypePlayerState::ServerSendSnapshotTaken_Implementation(AHypePlayerState* toPlayer, int32 atMeasure)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSendSnapshotTaken(this, toPlayer, atMeasure);
    }
}
bool AHypePlayerState::ServerSendSnapshotTaken_Validate(AHypePlayerState* toPlayer, int32 atMeasure) { return true; }

// FUN_140696820 (shipping): only a request to the current DJ (MPS +0x320) goes into RequestMgr's list (FUN_140651ce0).
void AHypePlayerState::ServerSubmitRequest_Implementation(const FMultiplayerLiveRequest& req, AHypePlayerState* toDJ)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSubmitRequest(this, req, toDJ);
    }
}
bool AHypePlayerState::ServerSubmitRequest_Validate(const FMultiplayerLiveRequest& req, AHypePlayerState* toDJ) { return true; }

// The master BPM / key / mode: the authority's clock takes the tempo and listeners get a
// GlobalAudioState item, which their consumer applies on arrival.
void AHypePlayerState::ServerUpdateGlobalAudioState_Implementation(int32 Tick, int32 BPM, EKey Key, EKeyMode mode)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnUpdateGlobalAudioState(this, Tick, BPM, Key, mode);
    }
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::GlobalAudioState, Tick);
    Item.MasterBpm = BPM;
    Item.MasterKey = Key;
    Item.MasterMode = mode;
    RecordAction(this, MoveTemp(Item));
}
bool AHypePlayerState::ServerUpdateGlobalAudioState_Validate(int32 Tick, int32 BPM, EKey Key, EKeyMode mode) { return true; }

// FUN_140695ac0: only the session leader may kick; the game session kicks the target with "kicked_by_server_leader".
void AHypePlayerState::ServerLeaderKickPlayer_Implementation(AHypePlayerState* ps)
{
    if (ps && _isSessionLeader)
    {
        if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
        {
            Authority->OnLeaderKickPlayer(this, ps);
        }
    }
}
bool AHypePlayerState::ServerLeaderKickPlayer_Validate(AHypePlayerState* ps) { return true; }

// FUN_140696880: flips HypeMultiplayerSettings.EnableAfkChecks for the whole server; FUSER checks nothing.
void AHypePlayerState::ServerToggleAFKChecks_Implementation()
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnToggleAfkChecks(this);
    }
}
bool AHypePlayerState::ServerToggleAFKChecks_Validate() { return true; }

// Empty in FUSER's shipping build; its dedicated server took the set length from the first player.
void AHypePlayerState::ServerSetCoopRoundNumTurnMeasures_Implementation(int32 numRounds, int32 turnMeasures)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSetCoopRoundNumTurnMeasures(this, numRounds, turnMeasures);
    }
}
bool AHypePlayerState::ServerSetCoopRoundNumTurnMeasures_Validate(int32 numRounds, int32 turnMeasures) { return true; }

// FUN_140696560 / FUN_140696520 / FUN_140696670 / FUN_140696200 / FUN_14069f760: plain stores.
void AHypePlayerState::ServerSetPlatform_Implementation(EHypeOnlinePlatform platform) { _platform = platform; }
bool AHypePlayerState::ServerSetPlatform_Validate(EHypeOnlinePlatform platform) { return true; }
void AHypePlayerState::ServerSetPlatformUserId_Implementation(const FString& platformUserId) { _platformUserId = platformUserId; }
bool AHypePlayerState::ServerSetPlatformUserId_Validate(const FString& platformUserId) { return true; }
void AHypePlayerState::ServerSetXP_Implementation(int32 xp) { _hypeXP = xp; }
bool AHypePlayerState::ServerSetXP_Validate(int32 xp) { return true; }
void AHypePlayerState::ServerSetDLCDownloadState_Implementation(EDLCDownloadState downloadState) {} // non-replicated server field
bool AHypePlayerState::ServerSetDLCDownloadState_Validate(EDLCDownloadState downloadState) { return true; }
// The client reports that it applied its state-of-the-world snapshot (FUN_140699ac0 -> FUN_14083e570); FUSER only
// stores it in a non-replicated server field (PS +0x1112). The authority stops rebuilding that player's snapshot.
void AHypePlayerState::_ServerSetHasSyncedStateOfTheWorld_Implementation(bool synced)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnSetHasSyncedStateOfTheWorld(this, synced);
    }
    else
    {
        UE_LOG(LogHypeServer, Display, TEXT("FDS: player %d synced the state of the world: %s"), PlayerId, synced ? TEXT("yes") : TEXT("no"));
    }
}
bool AHypePlayerState::_ServerSetHasSyncedStateOfTheWorld_Validate(bool synced) { return true; }

// ---------------------------------------------------------------------------------------------- UDJPlayerState

// FUN_14062bd20 / FUN_14062bde0 / vtable +0x3e8: copy the configuration (replicated to the other players).
void UDJPlayerState::ServerSetAvatarConfig_Implementation(const FNetAvatarConfig& NetAvatarConfig) { _netAvatarConfig = NetAvatarConfig; }
void UDJPlayerState::ServerSetCrate_Implementation(const FCrateData& CrateData) { _crateData = CrateData; }
void UDJPlayerState::ServerSetVenueConfig_Implementation(const FVenueConfigKeys& VenueConfig) { _venueConfig = VenueConfig; }

// ---------------------------------------------------------------------------------------------- AHypePlayerController

// FUN_14069f6e0: answer the client's clock-sync request with the server world time.
void AHypePlayerController::_ServerRequestServerTime_Implementation(float requestWorldTime)
{
    const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    _ClientReportServerTime(requestWorldTime, GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds());
}
bool AHypePlayerController::_ServerRequestServerTime_Validate(float requestWorldTime) { return true; }

// FUN_140696b40: the client reports its idle seconds; the server kicks an idle DJ (reason "afk_dj") past the threshold.
void AHypePlayerController::ServerUpdatePlayerAfkSeconds_Implementation(int32 afkSeconds)
{
    if (IHypeServerAuthority* Authority = IHypeServerAuthority::Get(this))
    {
        Authority->OnUpdateAfkSeconds(this, afkSeconds);
    }
}
bool AHypePlayerController::ServerUpdatePlayerAfkSeconds_Validate(int32 afkSeconds) { return true; }
