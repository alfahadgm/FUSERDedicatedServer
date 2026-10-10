// Generated from FUSER's recorded network metadata - do not edit by hand.
#include "AudioEffectData.h"
#include "CelData.h"
#include "CelMix.h"
#include "CelSongSet.h"
#include "DJPlayerState.h"
#include "HypeGameModeBase.h"
#include "GameplayGameModeBase.h"
#include "HypeActor.h"
#include "HypeActorComponent.h"
#include "HypeGameState.h"
#include "HypeGameStateVenue.h"
#include "HypeGameStateMainStage.h"
#include "HypeGameStateMultiplayerFreestyle.h"
#include "HypeHUD.h"
#include "HypePawn.h"
#include "HypePlayerController.h"
#include "HypePlayerState.h"
#include "HypeWorldSettings.h"
#include "InstrumentCelData.h"
#include "MultiplayerGameplayState.h"
#include "MainStageMPGameplayState.h"
#include "MomentItem.h"
#include "MusicTimelineState.h"
#include "RequestMgr.h"
#include "ResultsState.h"
#include "ShellPawn.h"
#include "Trick.h"
#include "VenueData.h"
#include "VenueGameMode.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeRpc, Log, All);
// A server RPC FDS does not implement yet: the first call, then a running count every 50 calls.
#define FDS_UNHANDLED_RPC(Name) do { static int32 Calls = 0; ++Calls; if (Calls == 1 || Calls % 50 == 0) { UE_LOG(LogHypeRpc, Warning, TEXT("FDS: unhandled server RPC %s (%d calls)"), Name, Calls); } } while (0)

// ---------------------------------------------------------------- UAudioEffectData
UAudioEffectData::UAudioEffectData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UCelData
UCelData::UCelData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UCelMix
UCelMix::UCelMix(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UCelSongSet
UCelSongSet::UCelSongSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UDJPlayerState
UDJPlayerState::UDJPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    _netAvatarConfig.SelectedBodyType = ECharacterBodyType::FemSkinny;
    _netAvatarConfig.SelectedOutfitParts.Init(0, 15);
    _netAvatarConfig.SelectedMaterialVariants.Init(0, 15);
    _netAvatarConfig.SelectedColorPaletteIndices.Init(0, 15);
    _netAvatarConfig.SelectedSkinMaterialVariant = 0;
    _netAvatarConfig.SelectedTextureAssetMakeup = 0;
    _netAvatarConfig.SelectedTextureAssetTattoo = 0;
    _netAvatarConfig.SelectedHeightScaleFactor = 1.0f;
    _netAvatarConfig.SelectedPersonality = ECharacterPersonality::Personality1;
    _netAvatarConfig.SelectedEmote = 0;
    _venueConfig.VenueConfigName = TEXT("");
    _venueConfig.VfxCan = NAME_None;
    _venueConfig.VfxBackdrop = NAME_None;
    _venueConfig.VfxAmbient = NAME_None;
    _venueConfig.ScreenItems.Init(NAME_None, 3);
    _venueConfig.OverrideScreenItem = NAME_None;
    _venueConfig.EnvSkyPackage = NAME_None;
    _venueConfig.CrowdParticles = NAME_None;
    _venueConfig.LightShow = NAME_None;
    _venueConfig.VenueConfigIndex = -1;
    _crateData.crateName = TEXT("");
    _crateData.NumSongSlots = 0;
    _crateData.NumInstrumentSlots = 0;
    _crateData.NumAudioEffectSlots = 0;
    _crateData.NumMomentSlots = 0;
    _crateData.MissionConstraint.NumSongSlots = -1;
    _crateData.MissionConstraint.MinSongs = -1;
    _crateData.MissionConstraint.NumInstrumentSlots = -1;
    _crateData.MissionConstraint.MinInstruments = -1;
    _crateData.MissionConstraint.NumAudioEffectSlots = -1;
    _crateData.MissionConstraint.MinAudioEffects = -1;
    _crateData.MissionConstraint.NumMomentSlots = -1;
    _crateData.MissionConstraint.MinMoments = -1;
    _crateData.crateIndex = -1;
    SetIsReplicatedByDefault(true);
}

void UDJPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UDJPlayerState, _netAvatarConfig, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(UDJPlayerState, _venueConfig, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(UDJPlayerState, _crateData, COND_SkipOwner);
}

void UDJPlayerState::OnRep_AvatarConfig() {}

void UDJPlayerState::OnRep_VenueConfig() {}

void UDJPlayerState::OnRep_CrateData() {}

// ---------------------------------------------------------------- AHypeGameModeBase
AHypeGameModeBase::AHypeGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AGameplayGameModeBase
AGameplayGameModeBase::AGameplayGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AHypeActor
AHypeActor::AHypeActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UHypeActorComponent
UHypeActorComponent::UHypeActorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AHypeGameState
AHypeGameState::AHypeGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    _autoSpectatorSettings.RhythmojiSpamEnabled = false;
    _autoSpectatorSettings.RhythmojiSpamRandomType = false;
    _autoSpectatorSettings.RhythmojiSpamStreakRandom = false;
    _autoSpectatorSettings.RhythmojiSpamOnBeat = false;
    _autoSpectatorSettings.RhythmojiSpamMessageChance = 0.1f;
    _autoSpectatorSettings.RhythmojiSpamRate = 1.0f;
    _autoSpectatorSettings.RhythmojiSpamType = EMultiplayerPraiseType::Heart;
    _autoSpectatorSettings.RhythmojiSpamStreak = 1;
    _autoSpectatorSettings.RequestSpamInstructionNum = 0;
    _autoSpectatorSettings.RequestSpamType = ERequestSpamInstructionType::None;
    _autoSpectatorSettings.RequestSpamTime = 0.0f;
    NetPriority = 5.0f; // FUSER CDO (parent 1.0)
    MusicTimelineState = CreateDefaultSubobject<UMusicTimelineState>(TEXT("MusicTimelineState"));
}

void AHypeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AHypeGameState, _autoSpectatorSettings, COND_None);
}

void AHypeGameState::NetMulticastPraises_Implementation(const TArray<FCollectedPraise>& praises) {}

void AHypeGameState::NetMulticastPraisesUnreliable_Implementation(const TArray<FCollectedPraise>& praises) {}

void AHypeGameState::NetMulticastSignatureMove_Implementation(AHypePlayerState* FromPlayer) {}

void AHypeGameState::NetMulticastSuperDropPerformed_Implementation(EBattlePlayerLabel battlePlayerLabel) {}

// ---------------------------------------------------------------- AHypeGameStateVenue
AHypeGameStateVenue::AHypeGameStateVenue(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AHypeGameStateMainStage
AHypeGameStateMainStage::AHypeGameStateMainStage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    MultiplayerGameplayState = CreateDefaultSubobject<UMainStageMPGameplayState>(TEXT("MainStageMPGameplayState"));
    RequestMgr = CreateDefaultSubobject<URequestMgr>(TEXT("RequestMgr"));
}

// ---------------------------------------------------------------- AHypeGameStateMultiplayerFreestyle
AHypeGameStateMultiplayerFreestyle::AHypeGameStateMultiplayerFreestyle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    MultiplayerGameplayState = CreateDefaultSubobject<UMultiplayerGameplayState>(TEXT("MultiplayerGameplayState"));
    RequestMgr = CreateDefaultSubobject<URequestMgr>(TEXT("RequestMgr"));
}

// ---------------------------------------------------------------- AHypeHUD
AHypeHUD::AHypeHUD(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AHypePawn
AHypePawn::AHypePawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bReplicates = false; // FUSER CDO (parent True)
}

// ---------------------------------------------------------------- AHypePlayerController
AHypePlayerController::AHypePlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiMessageChance_Implementation(float chance) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiMessageChance")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamEnabled_Implementation(bool Enabled) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamEnabled")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamOnBeat_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamOnBeat")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamRandomStreak_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamRandomStreak")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamRate_Implementation(float perSec) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamRate")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamStreak_Implementation(int32 Streak) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamStreak")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamType_Implementation(EMultiplayerPraiseType Type) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamType")); }

void AHypePlayerController::CHEAT_AutoSpec_SetRhythmojiSpamTypeRandom_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SetRhythmojiSpamTypeRandom")); }

void AHypePlayerController::CHEAT_AutoSpec_SubmitRequestIn_Implementation(float Time) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SubmitRequestIn")); }

void AHypePlayerController::CHEAT_AutoSpec_SubmitRequestRandom_Implementation(float Time) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_AutoSpec_SubmitRequestRandom")); }

void AHypePlayerController::CHEAT_ServerDumpObjList_Implementation(const FString& ClassName) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerDumpObjList")); }
bool AHypePlayerController::CHEAT_ServerDumpObjList_Validate(const FString& ClassName) { return true; }

void AHypePlayerController::CHEAT_ServerEnableStatReplicator_Implementation(bool Enabled) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerEnableStatReplicator")); }

void AHypePlayerController::CHEAT_ServerSetQueryStatsEnabled_Implementation(bool Enabled) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerSetQueryStatsEnabled")); }
bool AHypePlayerController::CHEAT_ServerSetQueryStatsEnabled_Validate(bool Enabled) { return true; }

void AHypePlayerController::CHEAT_ServerSetTickRate_Implementation(int32 tickRate) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerSetTickRate")); }
bool AHypePlayerController::CHEAT_ServerSetTickRate_Validate(int32 tickRate) { return true; }

void AHypePlayerController::CHEAT_ServerStartNetProfiling_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerStartNetProfiling")); }
bool AHypePlayerController::CHEAT_ServerStartNetProfiling_Validate() { return true; }

void AHypePlayerController::CHEAT_ServerStartStat_Implementation(const FString& baseFileName) { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerStartStat")); }
bool AHypePlayerController::CHEAT_ServerStartStat_Validate(const FString& baseFileName) { return true; }

void AHypePlayerController::CHEAT_ServerStopNetProfiling_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerStopNetProfiling")); }
bool AHypePlayerController::CHEAT_ServerStopNetProfiling_Validate() { return true; }

void AHypePlayerController::CHEAT_ServerStopStat_Implementation() { FDS_UNHANDLED_RPC(TEXT("HypePlayerController.CHEAT_ServerStopStat")); }
bool AHypePlayerController::CHEAT_ServerStopStat_Validate() { return true; }

void AHypePlayerController::_ClientReportServerTime_Implementation(float requestWorldTime, float serverTime) {}

// ---------------------------------------------------------------- AHypePlayerState
AHypePlayerState::AHypePlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    _hypeUserId = -1;
    _platformUserId = TEXT("");
    _platform = EHypeOnlinePlatform::NumPlatforms;
    _hypeXP = 0;
    _team = EMultiplayerTeam::Unknown;
    _djIndex = -1;
    _stateOfTheWorld.AtMasterTick = -1;
    _stateOfTheWorld.InRound = -1;
    _isSessionLeader = false;
    _isReadyCheck = EHypePlayerStateIsReadyFlags::NoneMask;
    _gameSessionId = TEXT("");
    _serverAccessState = EServerAccessState::NotSet;
    _mainStageVoiceChatGroupID = -1;
    _resultsState = CreateDefaultSubobject<UResultsState>(TEXT("ResultsState"));
}

void AHypePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _hypeUserId, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _platformUserId, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _platform, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _hypeXP, COND_Custom);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _team, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _djIndex, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _multiplayerActions, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _multiplayerFXParams, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _multiplayerInstrumentParams, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _stateOfTheWorld, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _isSessionLeader, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _isReadyCheck, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _gameSessionId, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _serverAccessState, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _mainStageVoiceChatGroupID, COND_None);
    DOREPLIFETIME_CONDITION(AHypePlayerState, _djPlayerState, COND_None);
}

void AHypePlayerState::OnRep_HypeUserId() {}

void AHypePlayerState::OnRep_PlatformUserId() {}

void AHypePlayerState::OnRep_XP() {}

void AHypePlayerState::OnRep_Team() {}

void AHypePlayerState::OnRep_DJIndex() {}

void AHypePlayerState::OnRep_MultiplayerActions() {}

void AHypePlayerState::OnRep_MultiplayerFXParams() {}

void AHypePlayerState::OnRep_MultiplayerInstrumentParams() {}

void AHypePlayerState::OnRep_StateOfTheWorld() {}

void AHypePlayerState::OnRep_IsSessionLeader() {}

void AHypePlayerState::OnRep_IsReadyCheck() {}

void AHypePlayerState::OnRep_GameSessionId() {}

void AHypePlayerState::OnRep_ServerAccessState() {}

void AHypePlayerState::OnRep_VoiceChatGroup() {}

void AHypePlayerState::OnRep_DJPlayerState() {}

void AHypePlayerState::CHEAT_ServerForceNumRoundsMeasures_Implementation(int32 rounds, int32 measures) { FDS_UNHANDLED_RPC(TEXT("HypePlayerState.CHEAT_ServerForceNumRoundsMeasures")); }

void AHypePlayerState::CHEAT_ServerRequestMultiplayerGameStart_Implementation(float startIn) { FDS_UNHANDLED_RPC(TEXT("HypePlayerState.CHEAT_ServerRequestMultiplayerGameStart")); }

void AHypePlayerState::ClientSendCraftedInstrumentCel_Implementation(const FSerializableCraftedInstrumentCel& Data) {}

void AHypePlayerState::ClientSendDataPair_Implementation(const FString& Key, const FString& Value) {}

void AHypePlayerState::ServerSendDataPair_Implementation(const FString& Key, const FString& Value) {}
bool AHypePlayerState::ServerSendDataPair_Validate(const FString& Key, const FString& Value) { return true; }

void AHypePlayerState::ServerSetAsDJ_Implementation() {}
bool AHypePlayerState::ServerSetAsDJ_Validate() { return true; }

void AHypePlayerState::ServerSetAsSpectator_Implementation() {}
bool AHypePlayerState::ServerSetAsSpectator_Validate() { return true; }

void AHypePlayerState::ServerSetBattleMatchmakingRating_Implementation(int32 battleMatchmakingRating) {}

void AHypePlayerState::ServerSetBattleNumDiscs_Implementation(int32 battleNumDiscs) {}

void AHypePlayerState::ServerSetInitialBlockedHypeUserIds_Implementation(const TArray<int32>& initialBlockedHypeUsers) {}
bool AHypePlayerState::ServerSetInitialBlockedHypeUserIds_Validate(const TArray<int32>& initialBlockedHypeUsers) { return true; }

void AHypePlayerState::ServerSetNumBattleMatchesLost_Implementation(int32 battleMatchesLost) {}

void AHypePlayerState::ServerSetNumBattleMatchesPlayed_Implementation(int32 battleMatchesPlayed) {}

void AHypePlayerState::ServerSetNumBattleMatchesWon_Implementation(int32 battleMatchesWon) {}

void AHypePlayerState::ServerToggleAutoReadyCheck_Implementation() {}
bool AHypePlayerState::ServerToggleAutoReadyCheck_Validate() { return true; }

void AHypePlayerState::ServerTogglePinDJ_Implementation() {}
bool AHypePlayerState::ServerTogglePinDJ_Validate() { return true; }

void AHypePlayerState::ServerUpdateVoiceChatGroup_Implementation(bool isSpectatorVoiceChatPermitted) {}
bool AHypePlayerState::ServerUpdateVoiceChatGroup_Validate(bool isSpectatorVoiceChatPermitted) { return true; }

// ---------------------------------------------------------------- AHypeWorldSettings
AHypeWorldSettings::AHypeWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bReplicates = false; // FUSER CDO (parent True)
}

// ---------------------------------------------------------------- UInstrumentCelData
UInstrumentCelData::UInstrumentCelData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UMultiplayerGameplayState
UMultiplayerGameplayState::UMultiplayerGameplayState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    MainStageRhythmojiRates.Init(0.0f, 23);
    _numCoopFreestyleRounds = -1;
    _coopTurnLengthMeasures = -1;
    _hottestMix.BPM = 128;
    _hottestMix.Key = EKey::C;
    _hottestMix.KeyMode = EKeyMode::Major;
    _hottestMix.IsRoundHottestMix = false;
    _hottestMix.HottestMixWeight = 0;
    _hottestMix.HottestMixPraise = 0;
    _hottestMix.HottestMixSnapshots = 0;
    _hottestMix.HottestMixRequests = 0;
    _serverShutdownWorldTime = -1.0f;
    _pinCurrentStage = false;
    SetIsReplicatedByDefault(true);
}

void UMultiplayerGameplayState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, MainStageRhythmojiRates, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _gameStages, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _numCoopFreestyleRounds, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _coopTurnLengthMeasures, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _hottestMix, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _serverShutdownWorldTime, COND_None);
    DOREPLIFETIME_CONDITION(UMultiplayerGameplayState, _pinCurrentStage, COND_None);
}

void UMultiplayerGameplayState::_OnRep_MainStageRhythmojiRates() {}

void UMultiplayerGameplayState::_OnRep_GameplayStages() {}

void UMultiplayerGameplayState::_OnRep_NumCoopFreestyleRounds() {}

void UMultiplayerGameplayState::_OnRep_CoopTurnLengthMeasures() {}

void UMultiplayerGameplayState::_OnRep_HottestMix() {}

void UMultiplayerGameplayState::_OnRep_ServerShutdownWorldTime() {}

// ---------------------------------------------------------------- UMainStageMPGameplayState
UMainStageMPGameplayState::UMainStageMPGameplayState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UMomentItem
UMomentItem::UMomentItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- UMusicTimelineState
UMusicTimelineState::UMusicTimelineState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    _syncState.roundIndex = 0;
    _syncState.ServerWorldSeconds = 0.0f;
    _syncState.ServerMasterTick = -1;
    _syncState.ServerBPM = -1;
    _syncState.ServerDefaultSyncTick = -1;
    SetIsReplicatedByDefault(true);
}

void UMusicTimelineState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMusicTimelineState, _syncState, COND_None);
}

void UMusicTimelineState::OnRep_SyncState() {}

// ---------------------------------------------------------------- URequestMgr
URequestMgr::URequestMgr(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
}

void URequestMgr::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(URequestMgr, _multiplayerLiveRequests, COND_None);
}

void URequestMgr::OnRep_MultiplayerLiveRequests() {}

// ---------------------------------------------------------------- UResultsState
UResultsState::UResultsState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    _results.TrickResults.TrickTimingCounts.Init(0, 5);
    _results.TrickResults.TotalTricks = 0;
    _results.PraiseResults.PraiseCounts.Init(0, 23);
    _results.PraiseResults.TotalPraise = 0;
    _results.PraiseResults.TotalNumFans = 0;
    _results.PraiseResults.PraiseSent = 0;
    _results.PraiseResults.MegaPraiseSent = 0;
    _results.SnapshotResults.TotalNumSnapshotsTaken = 0;
    _results.RequestsResults.RequestsCounts.Init(0, 2);
    _results.RequestsResults.TotalNumRequestsReceived = 0;
    _results.MyHottestMix.BPM = 128;
    _results.MyHottestMix.Key = EKey::C;
    _results.MyHottestMix.KeyMode = EKeyMode::Major;
    _results.MyHottestMix.IsRoundHottestMix = false;
    _results.MyHottestMix.HottestMixWeight = 0;
    _results.MyHottestMix.HottestMixPraise = 0;
    _results.MyHottestMix.HottestMixSnapshots = 0;
    _results.MyHottestMix.HottestMixRequests = 0;
    _results.BattleResults.HasReceivedBattleResults = false;
    _results.BattleResults.TotalPerfectDownbeatDrops = 0;
    _results.BattleResults.TotalPerfectPickupDrops = 0;
    _results.BattleResults.TotalPerfectPhraseDrops = 0;
    _results.BattleResults.TotalRequestsFulfilled = 0;
    _results.BattleResults.IsWinner = false;
    _results.BattleResults.IsRankedMatch = false;
    _results.BattleResults.PreviousNumDiscs = -1;
    _results.BattleResults.NewNumDiscs = -1;
    _results.BattleResults.DiscDelta = 0;
    _results.BattleResults.PreviousNumDiscsInRank = -1;
    _results.BattleResults.NewNumDiscsInRank = -1;
    _results.BattleResults.OldMMR = -1;
    _results.BattleResults.UpdatedMMR = -1;
    _results.BattleResults.StreakWinCount = 0;
    _results.BattleResults.StreakLossCount = 0;
    _results.BattleResults.TotalBattleWins = 0;
    _results.BattleResults.TotalBattleLosses = 0;
    _results.BattleResults.PreviousTier = EBattleRankingTier::Beginner;
    _results.BattleResults.NewTier = EBattleRankingTier::Beginner;
    _results.BattleResults.TierDelta = 0;
    _results.BattleResults.PreviousRank = -1;
    _results.BattleResults.NewRank = -1;
    _results.BattleResults.RankDelta = 0;
    _mainStageResults.MyHottestMix.BPM = 128;
    _mainStageResults.MyHottestMix.Key = EKey::C;
    _mainStageResults.MyHottestMix.KeyMode = EKeyMode::Major;
    _mainStageResults.MyHottestMix.IsRoundHottestMix = false;
    _mainStageResults.MyHottestMix.HottestMixWeight = 0;
    _mainStageResults.MyHottestMix.HottestMixPraise = 0;
    _mainStageResults.MyHottestMix.HottestMixSnapshots = 0;
    _mainStageResults.MyHottestMix.HottestMixRequests = 0;
    _mainStageResults.PraiseResults.PraiseCounts.Init(0, 23);
    _mainStageResults.PraiseResults.TotalPraise = 0;
    _mainStageResults.PraiseResults.TotalNumFans = 0;
    _mainStageResults.PraiseResults.PraiseSent = 0;
    _mainStageResults.PraiseResults.MegaPraiseSent = 0;
}

void UResultsState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UResultsState, _results, COND_Custom);
    DOREPLIFETIME_CONDITION(UResultsState, _mainStageResults, COND_Custom);
}

void UResultsState::OnRep_Results() {}

void UResultsState::OnRep_MainStageResults() {}

// ---------------------------------------------------------------- AShellPawn
AShellPawn::AShellPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- ATrick
ATrick::ATrick(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bReplicates = true; // FUSER CDO (parent False)
    NetDormancy = DORM_Initial; // FUSER CDO (parent 'DORM_Awake')
}

// ---------------------------------------------------------------- UVenueData
UVenueData::UVenueData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------- AVenueGameMode
AVenueGameMode::AVenueGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}
