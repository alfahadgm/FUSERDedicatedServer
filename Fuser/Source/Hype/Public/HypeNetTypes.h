// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "HypeEnums.h"
#include "OnlineSubsystemHypeEnums.h"
#include "HypeNetTypes.generated.h"

class AHypePlayerState;
class ATrick;
class UAudioEffectData;
class UCelSongSet;
class UInstrumentCelData;
class UMomentItem;
class UTexture2D;


// /Script/Hype.AudioEffectInfo: FUSER size 56, flags 0x00001401
USTRUCT(BlueprintType)
struct FAudioEffectInfo
{
    GENERATED_BODY()

    UPROPERTY()
    UAudioEffectData* effectData;

    UPROPERTY()
    TArray<float> Params;

    UPROPERTY()
    bool IsEffectEnabled;

    UPROPERTY()
    TArray<bool> AppliedToDecks;

    UPROPERTY()
    int32 OwnedByHypeUserId;

    FAudioEffectInfo()
    {
        effectData = nullptr;
        IsEffectEnabled = false;
        OwnedByHypeUserId = 0;
        effectData = nullptr;
        IsEffectEnabled = false;
        OwnedByHypeUserId = 0;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FAudioEffectInfo> : public TStructOpsTypeTraitsBase2<FAudioEffectInfo>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.AutospectatorSettings: FUSER size 32, flags 0x00001001
USTRUCT(BlueprintType)
struct FAutospectatorSettings
{
    GENERATED_BODY()

    UPROPERTY()
    bool RhythmojiSpamEnabled;

    UPROPERTY()
    bool RhythmojiSpamRandomType;

    UPROPERTY()
    bool RhythmojiSpamStreakRandom;

    UPROPERTY()
    bool RhythmojiSpamOnBeat;

    UPROPERTY()
    float RhythmojiSpamMessageChance;

    UPROPERTY()
    float RhythmojiSpamRate;

    UPROPERTY()
    EMultiplayerPraiseType RhythmojiSpamType;

    UPROPERTY()
    int32 RhythmojiSpamStreak;

    UPROPERTY()
    int32 RequestSpamInstructionNum;

    UPROPERTY()
    ERequestSpamInstructionType RequestSpamType;

    UPROPERTY()
    float RequestSpamTime;

    FAutospectatorSettings()
    {
        RhythmojiSpamEnabled = false;
        RhythmojiSpamRandomType = false;
        RhythmojiSpamStreakRandom = false;
        RhythmojiSpamOnBeat = false;
        RhythmojiSpamMessageChance = 0.0f;
        RhythmojiSpamRate = 0.0f;
        RhythmojiSpamType = (EMultiplayerPraiseType)0;
        RhythmojiSpamStreak = 0;
        RequestSpamInstructionNum = 0;
        RequestSpamType = (ERequestSpamInstructionType)0;
        RequestSpamTime = 0.0f;
        RhythmojiSpamEnabled = false;
        RhythmojiSpamRandomType = false;
        RhythmojiSpamStreakRandom = false;
        RhythmojiSpamOnBeat = false;
        RhythmojiSpamMessageChance = 0.00f;
        RhythmojiSpamRate = 0.00f;
        RhythmojiSpamType = EMultiplayerPraiseType::ChefsKiss;
        RhythmojiSpamStreak = 0;
        RequestSpamInstructionNum = 0;
        RequestSpamType = ERequestSpamInstructionType::None;
        RequestSpamTime = 0.00f;
    }

};


// /Script/Hype.BattlePlayerSuperDropData: FUSER size 32, flags 0x00001001
USTRUCT(BlueprintType)
struct FBattlePlayerSuperDropData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> CelShortNames;

    UPROPERTY()
    TArray<int32> PowerLevels;

    FBattlePlayerSuperDropData()
    {
    }

};


// /Script/Hype.BattleResults: FUSER size 128, flags 0x00001001
USTRUCT(BlueprintType)
struct FBattleResults
{
    GENERATED_BODY()

    UPROPERTY()
    bool HasReceivedBattleResults;

    UPROPERTY()
    int32 TotalPerfectDownbeatDrops;

    UPROPERTY()
    int32 TotalPerfectPickupDrops;

    UPROPERTY()
    int32 TotalPerfectPhraseDrops;

    UPROPERTY()
    int32 TotalRequestsFulfilled;

    UPROPERTY()
    FBattlePlayerSuperDropData BestSuperDrop;

    UPROPERTY()
    bool IsWinner;

    UPROPERTY()
    bool IsRankedMatch;

    UPROPERTY()
    int32 PreviousNumDiscs;

    UPROPERTY()
    int32 NewNumDiscs;

    UPROPERTY()
    int32 DiscDelta;

    UPROPERTY()
    int32 PreviousNumDiscsInRank;

    UPROPERTY()
    int32 NewNumDiscsInRank;

    UPROPERTY()
    int32 OldMMR;

    UPROPERTY()
    int32 UpdatedMMR;

    UPROPERTY()
    int32 StreakWinCount;

    UPROPERTY()
    int32 StreakLossCount;

    UPROPERTY()
    int32 TotalBattleWins;

    UPROPERTY()
    int32 TotalBattleLosses;

    UPROPERTY()
    EBattleRankingTier PreviousTier;

    UPROPERTY()
    EBattleRankingTier NewTier;

    UPROPERTY()
    int32 TierDelta;

    UPROPERTY()
    int32 PreviousRank;

    UPROPERTY()
    int32 NewRank;

    UPROPERTY()
    int32 RankDelta;

    FBattleResults()
    {
        HasReceivedBattleResults = false;
        TotalPerfectDownbeatDrops = 0;
        TotalPerfectPickupDrops = 0;
        TotalPerfectPhraseDrops = 0;
        TotalRequestsFulfilled = 0;
        IsWinner = false;
        IsRankedMatch = false;
        PreviousNumDiscs = 0;
        NewNumDiscs = 0;
        DiscDelta = 0;
        PreviousNumDiscsInRank = 0;
        NewNumDiscsInRank = 0;
        OldMMR = 0;
        UpdatedMMR = 0;
        StreakWinCount = 0;
        StreakLossCount = 0;
        TotalBattleWins = 0;
        TotalBattleLosses = 0;
        PreviousTier = (EBattleRankingTier)0;
        NewTier = (EBattleRankingTier)0;
        TierDelta = 0;
        PreviousRank = 0;
        NewRank = 0;
        RankDelta = 0;
        HasReceivedBattleResults = false;
        TotalPerfectDownbeatDrops = 0;
        TotalPerfectPickupDrops = 0;
        TotalPerfectPhraseDrops = 0;
        TotalRequestsFulfilled = 0;
        IsWinner = false;
        IsRankedMatch = false;
        PreviousNumDiscs = 0;
        NewNumDiscs = 0;
        DiscDelta = 0;
        PreviousNumDiscsInRank = 0;
        NewNumDiscsInRank = 0;
        OldMMR = 0;
        UpdatedMMR = 0;
        StreakWinCount = 0;
        StreakLossCount = 0;
        TotalBattleWins = 0;
        TotalBattleLosses = 0;
        PreviousTier = EBattleRankingTier::Beginner;
        NewTier = EBattleRankingTier::Beginner;
        TierDelta = 0;
        PreviousRank = 0;
        NewRank = 0;
        RankDelta = 0;
    }

};


// /Script/Hype.CollectedPraise: FUSER size 40, flags 0x00001201
USTRUCT(BlueprintType)
struct HYPE_API FCollectedPraise
{
    GENERATED_BODY()

    UPROPERTY()
    bool IsPraiseMessage;

    UPROPERTY()
    AHypePlayerState* ForPlayer;

    UPROPERTY()
    AHypePlayerState* FromPlayer;

    UPROPERTY()
    int32 Measure;

    UPROPERTY()
    EMultiplayerPraiseType praiseType;

    UPROPERTY()
    int32 Streak;

    UPROPERTY()
    int32 TextIndex;

    FCollectedPraise()
    {
        IsPraiseMessage = false;
        ForPlayer = nullptr;
        FromPlayer = nullptr;
        Measure = 0;
        praiseType = (EMultiplayerPraiseType)0;
        Streak = 0;
        TextIndex = 0;
        IsPraiseMessage = false;
        ForPlayer = nullptr;
        FromPlayer = nullptr;
        Measure = 0;
        praiseType = EMultiplayerPraiseType::ChefsKiss;
        Streak = 0;
        TextIndex = 0;
    }

};


// /Script/Hype.CrateConstraint: FUSER size 208, flags 0x00001001
USTRUCT(BlueprintType)
struct FCrateConstraint
{
    GENERATED_BODY()

    UPROPERTY()
    int32 NumSongSlots;

    UPROPERTY()
    int32 MinSongs;

    UPROPERTY()
    int32 NumInstrumentSlots;

    UPROPERTY()
    int32 MinInstruments;

    UPROPERTY()
    int32 NumAudioEffectSlots;

    UPROPERTY()
    int32 MinAudioEffects;

    UPROPERTY()
    int32 NumMomentSlots;

    UPROPERTY()
    int32 MinMoments;

    UPROPERTY()
    TArray<UCelSongSet*> RequiredSongs;

    UPROPERTY()
    TArray<UCelSongSet*> AdditionalDefaultSongs;

    UPROPERTY()
    TArray<UCelSongSet*> BannedSongs;

    UPROPERTY()
    TArray<UInstrumentCelData*> RequiredInstruments;

    UPROPERTY()
    TArray<UInstrumentCelData*> AdditionalDefaultInstruments;

    UPROPERTY()
    TArray<UAudioEffectData*> RequiredAudioEffects;

    UPROPERTY()
    TArray<UAudioEffectData*> AdditionalDefaultAudioEffects;

    UPROPERTY()
    TArray<EGenre> ConstrainToGenres;

    UPROPERTY()
    TArray<EGenre> BannedGenres;

    UPROPERTY()
    TArray<EYearRange> ConstrainToYearRanges;

    UPROPERTY()
    TArray<EYearRange> BannedYearRanges;

    FCrateConstraint()
    {
        NumSongSlots = 0;
        MinSongs = 0;
        NumInstrumentSlots = 0;
        MinInstruments = 0;
        NumAudioEffectSlots = 0;
        MinAudioEffects = 0;
        NumMomentSlots = 0;
        MinMoments = 0;
        NumSongSlots = 0;
        MinSongs = 0;
        NumInstrumentSlots = 0;
        MinInstruments = 0;
        NumAudioEffectSlots = 0;
        MinAudioEffects = 0;
        NumMomentSlots = 0;
        MinMoments = 0;
    }

};


// /Script/Hype.CrateData: FUSER size 368, flags 0x00001001
USTRUCT(BlueprintType)
struct FCrateData
{
    GENERATED_BODY()

    UPROPERTY()
    FString crateName;

    UPROPERTY()
    TSoftObjectPtr<UTexture2D> crateImage;

    UPROPERTY()
    TArray<TSoftObjectPtr<UCelSongSet>> songs;

    UPROPERTY()
    TArray<UInstrumentCelData*> instruments;

    UPROPERTY()
    TArray<UAudioEffectData*> audioEffects;

    UPROPERTY()
    TArray<UMomentItem*> moments;

    UPROPERTY()
    TArray<FName> momentCelShortNames;

    UPROPERTY()
    int32 NumSongSlots;

    UPROPERTY()
    int32 NumInstrumentSlots;

    UPROPERTY()
    int32 NumAudioEffectSlots;

    UPROPERTY()
    int32 NumMomentSlots;

    UPROPERTY()
    FCrateConstraint MissionConstraint;

    UPROPERTY()
    int32 crateIndex;

    FCrateData()
    {
        NumSongSlots = 0;
        NumInstrumentSlots = 0;
        NumAudioEffectSlots = 0;
        NumMomentSlots = 0;
        crateIndex = 0;
        NumSongSlots = 0;
        NumInstrumentSlots = 0;
        NumAudioEffectSlots = 0;
        NumMomentSlots = 0;
        crateIndex = 0;
    }

};


// /Script/Hype.DeckMuteSoloState: FUSER size 2, flags 0x00001401
USTRUCT(BlueprintType)
struct FDeckMuteSoloState
{
    GENERATED_BODY()

    UPROPERTY()
    bool isMutePressed;

    UPROPERTY()
    bool isSoloPressed;

    FDeckMuteSoloState()
    {
        isMutePressed = false;
        isSoloPressed = false;
        isMutePressed = false;
        isSoloPressed = false;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FDeckMuteSoloState> : public TStructOpsTypeTraitsBase2<FDeckMuteSoloState>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.TrickResults: FUSER size 24, flags 0x00001001
USTRUCT(BlueprintType)
struct FTrickResults
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> TrickTimingCounts;

    UPROPERTY()
    int32 TotalTricks;

    FTrickResults()
    {
        TotalTricks = 0;
        TotalTricks = 0;
    }

};


// /Script/Hype.PraiseResults: FUSER size 56, flags 0x00001001
USTRUCT(BlueprintType)
struct FPraiseResults
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> PraiseCounts;

    UPROPERTY()
    int32 TotalPraise;

    UPROPERTY()
    TArray<AHypePlayerState*> Fans;

    UPROPERTY()
    int32 TotalNumFans;

    UPROPERTY()
    int32 PraiseSent;

    UPROPERTY()
    int32 MegaPraiseSent;

    FPraiseResults()
    {
        TotalPraise = 0;
        TotalNumFans = 0;
        PraiseSent = 0;
        MegaPraiseSent = 0;
        TotalPraise = 0;
        TotalNumFans = 0;
        PraiseSent = 0;
        MegaPraiseSent = 0;
    }

};


// /Script/Hype.SnapshotResults: FUSER size 4, flags 0x00001001
USTRUCT(BlueprintType)
struct FSnapshotResults
{
    GENERATED_BODY()

    UPROPERTY()
    int32 TotalNumSnapshotsTaken;

    FSnapshotResults()
    {
        TotalNumSnapshotsTaken = 0;
        TotalNumSnapshotsTaken = 0;
    }

};


// /Script/Hype.RequestsResults: FUSER size 24, flags 0x00001001
USTRUCT(BlueprintType)
struct FRequestsResults
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> RequestsCounts;

    UPROPERTY()
    int32 TotalNumRequestsReceived;

    FRequestsResults()
    {
        TotalNumRequestsReceived = 0;
        TotalNumRequestsReceived = 0;
    }

};


// /Script/Hype.PerMeasureResultSingleRoundStat: FUSER size 16, flags 0x00001001
USTRUCT(BlueprintType)
struct FPerMeasureResultSingleRoundStat
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> Stat;

    FPerMeasureResultSingleRoundStat()
    {
    }

};


// /Script/Hype.PerMeasureResults: FUSER size 48, flags 0x00001001
USTRUCT(BlueprintType)
struct FPerMeasureResults
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPerMeasureResultSingleRoundStat> RequestsCompleted;

    UPROPERTY()
    TArray<FPerMeasureResultSingleRoundStat> PraiseReceived;

    UPROPERTY()
    TArray<FPerMeasureResultSingleRoundStat> SnapshotsReceived;

    FPerMeasureResults()
    {
    }

};


// /Script/Hype.MultiplayerMeasureHistoryEntry: FUSER size 72, flags 0x00001001
USTRUCT(BlueprintType)
struct FMultiplayerMeasureHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> CelShortNames;

    UPROPERTY()
    int32 BPM;

    UPROPERTY()
    EKey Key;

    UPROPERTY()
    EKeyMode KeyMode;

    UPROPERTY()
    AHypePlayerState* DJForMeasure;

    UPROPERTY()
    TArray<FAudioEffectInfo> AudioEffectState;

    UPROPERTY()
    bool IsRoundHottestMix;

    UPROPERTY()
    int32 HottestMixWeight;

    UPROPERTY()
    int32 HottestMixPraise;

    UPROPERTY()
    int32 HottestMixSnapshots;

    UPROPERTY()
    int32 HottestMixRequests;

    FMultiplayerMeasureHistoryEntry()
    {
        BPM = 0;
        Key = (EKey)0;
        KeyMode = (EKeyMode)0;
        DJForMeasure = nullptr;
        IsRoundHottestMix = false;
        HottestMixWeight = 0;
        HottestMixPraise = 0;
        HottestMixSnapshots = 0;
        HottestMixRequests = 0;
        BPM = 0;
        Key = EKey::C;
        KeyMode = EKeyMode::Major;
        DJForMeasure = nullptr;
        IsRoundHottestMix = false;
        HottestMixWeight = 0;
        HottestMixPraise = 0;
        HottestMixSnapshots = 0;
        HottestMixRequests = 0;
    }

};


// /Script/Hype.GameplayResults: FUSER size 360, flags 0x00001001
USTRUCT(BlueprintType)
struct FGameplayResults
{
    GENERATED_BODY()

    UPROPERTY()
    FTrickResults TrickResults;

    UPROPERTY()
    FPraiseResults PraiseResults;

    UPROPERTY()
    FSnapshotResults SnapshotResults;

    UPROPERTY()
    FRequestsResults RequestsResults;

    UPROPERTY()
    FPerMeasureResults PerMeasureResults;

    UPROPERTY()
    FMultiplayerMeasureHistoryEntry MyHottestMix;

    UPROPERTY()
    FBattleResults BattleResults;

    FGameplayResults()
    {
    }

};


// /Script/Hype.GridInstrumentCraftingState: FUSER size 16, flags 0x00001401
USTRUCT(BlueprintType)
struct FGridInstrumentCraftingState
{
    GENERATED_BODY()

    UPROPERTY()
    bool PrimaryActionOn;

    UPROPERTY()
    bool SecondaryActionOn;

    UPROPERTY()
    FVector2D GridPos;

    UPROPERTY()
    EGridInstrumentMode mode;

    FGridInstrumentCraftingState()
    {
        PrimaryActionOn = false;
        SecondaryActionOn = false;
        mode = (EGridInstrumentMode)0;
        PrimaryActionOn = false;
        SecondaryActionOn = false;
        mode = EGridInstrumentMode::Crafting;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FGridInstrumentCraftingState> : public TStructOpsTypeTraitsBase2<FGridInstrumentCraftingState>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.MainStageResults: FUSER size 128, flags 0x00001001
USTRUCT(BlueprintType)
struct FMainStageResults
{
    GENERATED_BODY()

    UPROPERTY()
    FMultiplayerMeasureHistoryEntry MyHottestMix;

    UPROPERTY()
    FPraiseResults PraiseResults;

    FMainStageResults()
    {
    }

};


// /Script/Hype.TrickAndTiming: FUSER size 16, flags 0x00001401
USTRUCT(BlueprintType)
struct FTrickAndTiming
{
    GENERATED_BODY()

    UPROPERTY()
    ATrick* Trick;

    UPROPERTY()
    ETrickTiming myTiming;

    FTrickAndTiming()
    {
        Trick = nullptr;
        myTiming = (ETrickTiming)0;
        Trick = nullptr;
        myTiming = ETrickTiming::Bad;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FTrickAndTiming> : public TStructOpsTypeTraitsBase2<FTrickAndTiming>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.MultiplayerActionItemEntry: FUSER size 256, flags 0x00001401
USTRUCT(BlueprintType)
struct FMultiplayerActionItemEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    EMultiplayerActionType ActionType;

    UPROPERTY()
    int32 UniqueId;

    UPROPERTY()
    int32 MasterTimelineTick;

    UPROPERTY()
    FName celShortName;

    UPROPERTY()
    ETrackType trackType;

    UPROPERTY()
    FTransform spawnTransform;

    UPROPERTY()
    EActionAfterDiscSpawn actionAfterDiscSpawn;

    UPROPERTY()
    int32 dropStartTick;

    UPROPERTY()
    int32 dropEndTick;

    UPROPERTY()
    bool isBattleSuperDrop;

    UPROPERTY()
    EAudioEffectSlot slot;

    UPROPERTY()
    FAudioEffectInfo SlotInfo;

    UPROPERTY()
    int32 MasterBpm;

    UPROPERTY()
    EKey MasterKey;

    UPROPERTY()
    EKeyMode MasterMode;

    UPROPERTY()
    TArray<FDeckMuteSoloState> DeckStates;

    UPROPERTY()
    float gain;

    UPROPERTY()
    EInstrumentCraftingSlot InstSlot;

    UPROPERTY()
    UInstrumentCelData* InstrumentCelData;

    UPROPERTY()
    int32 uniqueRequestId;

    UPROPERTY()
    EMultiplayerLiveRequestCompletionType completionType;

    UPROPERTY()
    FTrickAndTiming Trick;

    FMultiplayerActionItemEntry()
    {
        ActionType = (EMultiplayerActionType)0;
        UniqueId = 0;
        MasterTimelineTick = 0;
        trackType = (ETrackType)0;
        actionAfterDiscSpawn = (EActionAfterDiscSpawn)0;
        dropStartTick = 0;
        dropEndTick = 0;
        isBattleSuperDrop = false;
        slot = (EAudioEffectSlot)0;
        MasterBpm = 0;
        MasterKey = (EKey)0;
        MasterMode = (EKeyMode)0;
        gain = 0.0f;
        InstSlot = (EInstrumentCraftingSlot)0;
        InstrumentCelData = nullptr;
        uniqueRequestId = 0;
        completionType = (EMultiplayerLiveRequestCompletionType)0;
        ActionType = EMultiplayerActionType::Invalid;
        UniqueId = 0;
        MasterTimelineTick = 0;
        trackType = ETrackType::Deck0;
        actionAfterDiscSpawn = EActionAfterDiscSpawn::Grab;
        dropStartTick = 0;
        dropEndTick = 0;
        isBattleSuperDrop = false;
        slot = EAudioEffectSlot::Slot0;
        MasterBpm = 0;
        MasterKey = EKey::C;
        MasterMode = EKeyMode::Major;
        gain = 0.00f;
        InstSlot = EInstrumentCraftingSlot::Slot0;
        InstrumentCelData = nullptr;
        uniqueRequestId = 0;
        completionType = EMultiplayerLiveRequestCompletionType::Passed;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FMultiplayerActionItemEntry> : public TStructOpsTypeTraitsBase2<FMultiplayerActionItemEntry>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.MultiplayerActionArray: FUSER size 280, flags 0x00101001
USTRUCT(BlueprintType)
struct FMultiplayerActionArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMultiplayerActionItemEntry> ActionItems;

    FMultiplayerActionArray()
    {
    }

    // FUSER delta-serializes this fast array natively; implemented in HypeNetSerializers.cpp
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

};


template<>
struct TStructOpsTypeTraits<FMultiplayerActionArray> : public TStructOpsTypeTraitsBase2<FMultiplayerActionArray>
{
    enum { WithNetDeltaSerializer = true };
};


// /Script/Hype.MultiplayerGameStage: FUSER size 52, flags 0x00001001
USTRUCT(BlueprintType)
struct FMultiplayerGameStage
{
    GENERATED_BODY()

    UPROPERTY()
    EMultiplayerGameplayStage Stage;

    UPROPERTY()
    EShellScreenType ShellScreen;

    UPROPERTY()
    EShellScreenType MainStageSpectatorAltShellScreen;

    UPROPERTY()
    float RoundRealtimeSeconds;

    UPROPERTY()
    bool ServerHasCompletedStage;

    UPROPERTY()
    float ServerWorldTimeStageEnd;

    UPROPERTY()
    int32 djIndex;

    UPROPERTY()
    int32 RoundNumber;

    UPROPERTY()
    bool DJForStageExistsOnServer;

    UPROPERTY()
    int32 startMeasure;

    UPROPERTY()
    int32 endMeasure;

    UPROPERTY()
    EBattlePlayerLabel StageWinner;

    UPROPERTY()
    int32 BattleRoundIdx;

    UPROPERTY()
    int32 BattleBPM;

    UPROPERTY()
    EKey BattleKey;

    UPROPERTY()
    EKeyMode BattleKeyMode;

    FMultiplayerGameStage()
    {
        Stage = (EMultiplayerGameplayStage)0;
        ShellScreen = (EShellScreenType)0;
        MainStageSpectatorAltShellScreen = (EShellScreenType)0;
        RoundRealtimeSeconds = 0.0f;
        ServerHasCompletedStage = false;
        ServerWorldTimeStageEnd = 0.0f;
        djIndex = 0;
        RoundNumber = 0;
        DJForStageExistsOnServer = false;
        startMeasure = 0;
        endMeasure = 0;
        StageWinner = (EBattlePlayerLabel)0;
        BattleRoundIdx = 0;
        BattleBPM = 0;
        BattleKey = (EKey)0;
        BattleKeyMode = (EKeyMode)0;
        Stage = EMultiplayerGameplayStage::None;
        ShellScreen = EShellScreenType::None;
        MainStageSpectatorAltShellScreen = EShellScreenType::None;
        RoundRealtimeSeconds = 0.00f;
        ServerHasCompletedStage = false;
        ServerWorldTimeStageEnd = 0.00f;
        djIndex = 0;
        RoundNumber = 0;
        DJForStageExistsOnServer = false;
        startMeasure = 0;
        endMeasure = 0;
        StageWinner = EBattlePlayerLabel::None;
        BattleRoundIdx = 0;
        BattleBPM = 0;
        BattleKey = EKey::C;
        BattleKeyMode = EKeyMode::Major;
    }

};


// /Script/Hype.MultiplayerLiveRequest: FUSER size 72, flags 0x00001001
USTRUCT(BlueprintType)
struct FMultiplayerLiveRequest
{
    GENERATED_BODY()

    UPROPERTY()
    EMultiplayerLiveRequestType Type;

    UPROPERTY()
    FString ContentName;

    UPROPERTY()
    int32 SuccessTextIdx;

    UPROPERTY()
    int32 FailureTextIdx;

    UPROPERTY()
    AHypePlayerState* ForPlayer;

    UPROPERTY()
    int32 startMeasure;

    UPROPERTY()
    int32 AvailabilityDuration;

    UPROPERTY()
    TArray<AHypePlayerState*> RequestedBy;

    UPROPERTY()
    int32 UniqueId;

    FMultiplayerLiveRequest()
    {
        Type = (EMultiplayerLiveRequestType)0;
        SuccessTextIdx = 0;
        FailureTextIdx = 0;
        ForPlayer = nullptr;
        startMeasure = 0;
        AvailabilityDuration = 0;
        UniqueId = 0;
        Type = EMultiplayerLiveRequestType::SongRequest;
        SuccessTextIdx = 0;
        FailureTextIdx = 0;
        ForPlayer = nullptr;
        startMeasure = 0;
        AvailabilityDuration = 0;
        UniqueId = 0;
    }

};


// /Script/Hype.MultiplayerUnreliableActionItemEntry: FUSER size 52, flags 0x00001401
USTRUCT(BlueprintType)
struct FMultiplayerUnreliableActionItemEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    EMultiplayerUnreliableActionType ActionType;

    UPROPERTY()
    int32 MasterTimelineTick;

    UPROPERTY()
    EAudioEffectSlot FXSlot;

    UPROPERTY()
    float paramX;

    UPROPERTY()
    float paramY;

    UPROPERTY()
    EInstrumentCraftingSlot InstSlot;

    UPROPERTY()
    FGridInstrumentCraftingState InstState;

    FMultiplayerUnreliableActionItemEntry()
    {
        ActionType = (EMultiplayerUnreliableActionType)0;
        MasterTimelineTick = 0;
        FXSlot = (EAudioEffectSlot)0;
        paramX = 0.0f;
        paramY = 0.0f;
        InstSlot = (EInstrumentCraftingSlot)0;
        ActionType = EMultiplayerUnreliableActionType::Invalid;
        MasterTimelineTick = 0;
        FXSlot = EAudioEffectSlot::Slot0;
        paramX = 0.00f;
        paramY = 0.00f;
        InstSlot = EInstrumentCraftingSlot::Slot0;
    }

    // FUSER serializes this struct natively; implemented in HypeNetSerializers.cpp
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};


template<>
struct TStructOpsTypeTraits<FMultiplayerUnreliableActionItemEntry> : public TStructOpsTypeTraitsBase2<FMultiplayerUnreliableActionItemEntry>
{
    enum { WithNetSerializer = true };
};


// /Script/Hype.MultiplayerUnreliableActionArray: FUSER size 280, flags 0x00101001
USTRUCT(BlueprintType)
struct FMultiplayerUnreliableActionArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMultiplayerUnreliableActionItemEntry> UnreliableActionItems;

    FMultiplayerUnreliableActionArray()
    {
    }

    // FUSER delta-serializes this fast array natively; implemented in HypeNetSerializers.cpp
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

};


template<>
struct TStructOpsTypeTraits<FMultiplayerUnreliableActionArray> : public TStructOpsTypeTraitsBase2<FMultiplayerUnreliableActionArray>
{
    enum { WithNetDeltaSerializer = true };
};


// /Script/Hype.NetAvatarConfig: FUSER size 80, flags 0x00001001
USTRUCT(BlueprintType)
struct FNetAvatarConfig
{
    GENERATED_BODY()

    UPROPERTY()
    ECharacterBodyType SelectedBodyType;

    UPROPERTY()
    TArray<int32> SelectedOutfitParts;

    UPROPERTY()
    TArray<int32> SelectedMaterialVariants;

    UPROPERTY()
    TArray<int32> SelectedColorPaletteIndices;

    UPROPERTY()
    int32 SelectedSkinMaterialVariant;

    UPROPERTY()
    int32 SelectedTextureAssetMakeup;

    UPROPERTY()
    int32 SelectedTextureAssetTattoo;

    UPROPERTY()
    float SelectedHeightScaleFactor;

    UPROPERTY()
    ECharacterPersonality SelectedPersonality;

    UPROPERTY()
    int32 SelectedEmote;

    FNetAvatarConfig()
    {
        SelectedBodyType = (ECharacterBodyType)0;
        SelectedSkinMaterialVariant = 0;
        SelectedTextureAssetMakeup = 0;
        SelectedTextureAssetTattoo = 0;
        SelectedHeightScaleFactor = 0.0f;
        SelectedPersonality = (ECharacterPersonality)0;
        SelectedEmote = 0;
        SelectedBodyType = ECharacterBodyType::FemSkinny;
        SelectedSkinMaterialVariant = 0;
        SelectedTextureAssetMakeup = 0;
        SelectedTextureAssetTattoo = 0;
        SelectedHeightScaleFactor = 0.00f;
        SelectedPersonality = ECharacterPersonality::Personality1;
        SelectedEmote = 0;
    }

};


// /Script/Hype.ReplicatedTimelineSynchronizationState: FUSER size 20, flags 0x00001001
USTRUCT(BlueprintType)
struct FReplicatedTimelineSynchronizationState
{
    GENERATED_BODY()

    UPROPERTY()
    int32 roundIndex;

    UPROPERTY()
    float ServerWorldSeconds;

    UPROPERTY()
    int32 ServerMasterTick;

    UPROPERTY()
    int32 ServerBPM;

    UPROPERTY()
    int32 ServerDefaultSyncTick;

    FReplicatedTimelineSynchronizationState()
    {
        roundIndex = 0;
        ServerWorldSeconds = 0.0f;
        ServerMasterTick = 0;
        ServerBPM = 0;
        ServerDefaultSyncTick = 0;
        roundIndex = 0;
        ServerWorldSeconds = 0.00f;
        ServerMasterTick = 0;
        ServerBPM = 0;
        ServerDefaultSyncTick = 0;
    }

};


// /Script/Hype.SerializableCraftedInstrumentCel: FUSER size 32, flags 0x00001001
USTRUCT(BlueprintType)
struct FSerializableCraftedInstrumentCel
{
    GENERATED_BODY()

    UPROPERTY()
    FName shortName;

    UPROPERTY()
    FName AssetShortName;

    UPROPERTY()
    TArray<uint8> RecordedDataBytes;

    FSerializableCraftedInstrumentCel()
    {
    }

};


// /Script/Hype.StateOfTheWorld: FUSER size 1976, flags 0x00001001
USTRUCT(BlueprintType)
struct FStateOfTheWorld
{
    GENERATED_BODY()

    UPROPERTY()
    int32 AtMasterTick;

    UPROPERTY()
    int32 InRound;

    UPROPERTY()
    FMultiplayerActionArray GlobalAudioState;

    UPROPERTY()
    FMultiplayerActionArray DeckSoloMuteStates;

    UPROPERTY()
    FMultiplayerActionArray ActiveGridInstruments;

    UPROPERTY()
    FMultiplayerActionArray DeckPlayingState;

    UPROPERTY()
    FMultiplayerActionArray DeckCuedState;

    UPROPERTY()
    FMultiplayerActionArray TrackGainState;

    UPROPERTY()
    FMultiplayerActionArray AudioEffectState;

    FStateOfTheWorld()
    {
        AtMasterTick = 0;
        InRound = 0;
        AtMasterTick = 0;
        InRound = 0;
    }

};


// /Script/Hype.VenueConfigKeys: FUSER size 96, flags 0x00001001
USTRUCT(BlueprintType)
struct FVenueConfigKeys
{
    GENERATED_BODY()

    UPROPERTY()
    FString VenueConfigName;

    UPROPERTY()
    FName VfxCan;

    UPROPERTY()
    FName VfxBackdrop;

    UPROPERTY()
    FName VfxAmbient;

    UPROPERTY()
    TArray<FName> ScreenItems;

    UPROPERTY()
    FName OverrideScreenItem;

    UPROPERTY()
    FName EnvSkyPackage;

    UPROPERTY()
    FName CrowdParticles;

    UPROPERTY()
    FName LightShow;

    UPROPERTY()
    int32 VenueConfigIndex;

    FVenueConfigKeys()
    {
        VenueConfigIndex = 0;
        VenueConfigIndex = 0;
    }

};

