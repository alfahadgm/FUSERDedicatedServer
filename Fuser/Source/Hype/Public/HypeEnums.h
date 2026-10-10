// Generated from FUSER's recorded network metadata - do not edit by hand.

#pragma once

#include "CoreMinimal.h"
#include "HypeEnums.generated.h"


// /Script/Hype.EActionAfterDiscSpawn (EnumClass)
UENUM(BlueprintType)
enum class EActionAfterDiscSpawn : uint8
{
    Grab = 0,
    Play = 1,
    Queue = 2,
    Num = 3
};

// /Script/Hype.EAudioEffectSlot (EnumClass)
UENUM(BlueprintType)
enum class EAudioEffectSlot : uint8
{
    Slot0 = 0,
    Slot1 = 1,
    SlotTransition = 2,
    Deck0Slot = 3,
    Deck1Slot = 4,
    Deck2Slot = 5,
    Deck3Slot = 6,
    MasterSlot = 7,
    Num = 8
};

// /Script/Hype.EBattlePlayerLabel (EnumClass)
UENUM(BlueprintType)
enum class EBattlePlayerLabel : uint8
{
    None = 0,
    PlayerA = 1,
    PlayerB = 2
};

// /Script/Hype.EBattleRankingTier (EnumClass)
UENUM(BlueprintType)
enum class EBattleRankingTier : uint8
{
    Beginner = 0,
    Bronze = 1,
    Silver = 2,
    Gold = 3,
    Platinum = 4,
    Legend = 5,
    MAX = 6
};

// /Script/Hype.ECharacterBodyType (EnumClass)
UENUM(BlueprintType)
enum class ECharacterBodyType : uint8
{
    FemSkinny = 0,
    FemAverage = 1,
    FemHeavy = 2,
    FemHourglass = 3,
    FemPear = 4,
    MasSkinny = 5,
    MasAverage = 6,
    MasHeavy = 7,
    Num = 8
};

// /Script/Hype.ECharacterPersonality (EnumClass)
UENUM(BlueprintType)
enum class ECharacterPersonality : uint8
{
    Personality1 = 0,
    Personality2 = 1,
    Personality3 = 2,
    Personality4 = 3,
    Personality5 = 4,
    Personality6 = 5,
    Num = 6
};

// /Script/Hype.EDLCDownloadState (EnumClass)
UENUM(BlueprintType)
enum class EDLCDownloadState : uint8
{
    NotStarted = 0,
    Downloading = 1,
    Complete = 2
};

// /Script/Hype.EGenre (EnumClass)
UENUM(BlueprintType)
enum class EGenre : uint8
{
    None = 0,
    Classical = 1,
    Country = 2,
    Dance = 3,
    HipHop = 4,
    LatinAndCaribbean = 5,
    Pop = 6,
    RnB = 7,
    Rock = 8,
    Num = 9
};

// /Script/Hype.EGridInstrumentMode (EnumClass)
UENUM(BlueprintType)
enum class EGridInstrumentMode : uint8
{
    Crafting = 0,
    Soloing = 1,
    MAX = 2
};

// /Script/Hype.EHypePlayerStateIsReadyFlags (EnumClass)
UENUM(BlueprintType)
enum class EHypePlayerStateIsReadyFlags : uint8
{
    Lobby = 1,
    Results = 2,
    NoneMask = 0
};

// /Script/Hype.EInstrumentCraftingSlot (EnumClass)
UENUM(BlueprintType)
enum class EInstrumentCraftingSlot : uint8
{
    Slot0 = 0,
    Slot1 = 1,
    Slot2 = 2,
    Slot3 = 3,
    Num = 4
};

// /Script/Hype.EKey (EnumClass)
UENUM(BlueprintType)
enum class EKey : uint8
{
    C = 0,
    Db = 1,
    D = 2,
    Eb = 3,
    E = 4,
    F = 5,
    Gb = 6,
    G = 7,
    Ab = 8,
    A = 9,
    Bb = 10,
    B = 11,
    Num = 12
};

// /Script/Hype.EKeyMode (EnumClass)
UENUM(BlueprintType)
enum class EKeyMode : uint8
{
    Major = 0,
    Minor = 1,
    Num = 2
};

// /Script/Hype.EMultiplayerActionType (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerActionType : uint8
{
    Invalid = 0,
    SpawnDisc = 1,
    EjectDisc = 2,
    HotSwapOnDeck = 3,
    DoDropQueue = 4,
    EjectQueue = 5,
    EffectSlotState = 6,
    GlobalAudioState = 7,
    SoloMuteStates = 8,
    TrackGain = 9,
    CurrentCraftedInstrument = 10,
    RequestComplete = 11,
    TrickToDeck = 12
};

// /Script/Hype.EMultiplayerGameplayStage (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerGameplayStage : uint8
{
    None = 0,
    Lobby = 1,
    RoundStart_SetupGameplay = 2,
    RoundStart_DJOrderIntro = 3,
    LoadStageKit = 4,
    UpcomingDJ = 5,
    Playing = 6,
    RoundComplete_FarShot = 7,
    RoundComplete_HottestMixIntro = 8,
    RoundComplete_HottestMixPlayback = 9,
    Results = 10,
    Results_XPPayout = 11,
    Battle_WaitForPlayers = 12,
    Battle_MatchIntro = 13,
    Battle_RoundStart = 14,
    Battle_Gameplay = 15,
    Battle_RoundEnd = 16,
    Battle_RoundContinue = 17,
    Battle_MatchEnd = 18,
    Battle_Results = 19,
    Battle_Results_XPPayout = 20,
    MAX = 21
};

// /Script/Hype.EMultiplayerLiveRequestCompletionType (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerLiveRequestCompletionType : uint8
{
    Passed = 0,
    Failed = 1,
    Num = 2
};

// /Script/Hype.EMultiplayerLiveRequestType (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerLiveRequestType : uint8
{
    SongRequest = 0,
    InstrumentRequest = 1,
    Genre = 2,
    YearRange = 3,
    Num = 4
};

// /Script/Hype.EMultiplayerPraiseType (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerPraiseType : uint8
{
    ChefsKiss = 0,
    Donut = 1,
    Fire = 2,
    HotDog = 3,
    ThumbsUp = 4,
    Popcorn = 5,
    PointUp = 6,
    Wink = 7,
    Crown = 8,
    Dragon = 9,
    FrenchFries = 10,
    BigSmile = 11,
    NailPolish = 12,
    Smile = 13,
    TopHat = 14,
    Wizard = 15,
    Diamond = 16,
    Breakfast = 17,
    Heart = 18,
    HeadExplode = 19,
    PartyTime = 20,
    Sparkles = 21,
    Unicorn = 22,
    Num = 23
};

// /Script/Hype.EMultiplayerTeam (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerTeam : uint8
{
    Unknown = 0,
    DJ = 1,
    Spectator = 2
};

// /Script/Hype.EMultiplayerUnreliableActionType (EnumClass)
UENUM(BlueprintType)
enum class EMultiplayerUnreliableActionType : uint8
{
    Invalid = 0,
    FXParam = 1,
    InstrumentParam = 2
};

// /Script/Hype.ERequestSpamInstructionType (EnumClass)
UENUM(BlueprintType)
enum class ERequestSpamInstructionType : uint8
{
    None = 0,
    Random = 1,
    In = 2
};

// /Script/Hype.EServerAccessState (EnumClass)
UENUM(BlueprintType)
enum class EServerAccessState : uint8
{
    NotSet = 0,
    PublicServer = 1,
    PrivateServer = 2
};

// /Script/Hype.EShellScreenType (EnumClass)
UENUM(BlueprintType)
enum class EShellScreenType : uint8
{
    None = 0,
    IntroCinematic = 1,
    TitleScreen = 2,
    MainMenu = 3,
    VenueSelect = 4,
    CrateSelect = 5,
    CrateEdit = 6,
    VenueCustomization = 7,
    CharacterCustomization = 8,
    LoginDisplay = 9,
    Gameplay = 10,
    ChallengeSelect = 11,
    Loading = 12,
    MissionRewards = 13,
    CollectionHome = 14,
    CollectionCrateSelect = 15,
    CollectionMusic = 16,
    CollectionCharacter = 17,
    CollectionVenueItem = 18,
    Pause = 19,
    WorkspaceTransitionsOptions = 20,
    WorkspaceGlobalControlsOptions = 21,
    WorkspaceInstrumentPanel = 22,
    RecordingViewing = 23,
    Loadout = 24,
    VideoRecording = 25,
    VideoRecordingFullScreen = 26,
    MultiplayerHub = 27,
    MultiplayerLobby = 28,
    VenueBrowseStopGap = 29,
    MultiplayerCreateSession = 30,
    MultiplayerResults = 31,
    MultiplayerVoiceChatOptions = 32,
    OutfitSelect = 33,
    CharacterEdit = 34,
    MixBrowser = 35,
    MixPlaybackComplete = 36,
    Profile = 37,
    StageSelect = 38,
    StageKitSelect = 39,
    SocialHub = 40,
    EventDetails = 41,
    CompletedEvents = 42,
    EventEvaluation = 43,
    EventMixEvalFinalize = 44,
    DiscGrabbed = 45,
    GameplayIntro = 46,
    ShareVideoIntro = 47,
    ShowBreak = 48,
    ShowResults = 49,
    ShowFailure = 50,
    GraphicsOptions = 51,
    DebugUITestScreen = 52,
    Store = 53,
    StoreDetails = 54,
    Settings = 55,
    MultiplayerLiveRequests = 56,
    Credits = 57,
    PaxDemoShowResults = 58,
    PaxDemoMarketingUpsell = 59,
    BlockedPlayers = 60,
    CalibrationTestScreen = 61,
    MultiplayerPraisePicker = 62,
    MultiplayerGameSessionInvite = 63,
    PrivacyPolicy = 64,
    EulaReader = 65,
    NotificationFeed = 66,
    CalibrationScreen = 67,
    LoadingShellScreen = 68,
    MultiplayerSynchronization = 69,
    DebugAssetCaptureTest = 70,
    DebugGameplayCaptureUtilities = 71,
    MultiplayerBattleResults = 72,
    MultiplayerBattleHub = 73,
    MultiplayerBattleJoinWait = 74,
    DebugBattleScreen = 75,
    DigitalDemoUpsell = 76,
    MultiplayerMainStageHub = 77
};

// /Script/Hype.ETrackType (EnumClass)
UENUM(BlueprintType)
enum class ETrackType : uint8
{
    Deck0 = 0,
    Deck1 = 1,
    Deck2 = 2,
    Deck3 = 3,
    Transition = 4,
    Audition0 = 5,
    Audition1 = 6,
    Audition2 = 7,
    Audition3 = 8,
    Instrument0 = 9,
    Instrument1 = 10,
    Instrument2 = 11,
    Instrument3 = 12,
    TransitionSong0 = 13,
    TransitionSong1 = 14,
    TransitionSong2 = 15,
    TransitionSong3 = 16,
    Deck0A = 17,
    Deck1A = 18,
    Deck2A = 19,
    Deck3A = 20,
    Deck0B = 21,
    Deck1B = 22,
    Deck2B = 23,
    Deck3B = 24,
    Num = 25,
    All = 26
};

// /Script/Hype.ETrickTiming (EnumClass)
UENUM(BlueprintType)
enum class ETrickTiming : uint8
{
    Bad = 0,
    Ok = 1,
    Early = 2,
    Late = 3,
    Perfect = 4,
    Num = 5
};

// /Script/Hype.EYearRange (EnumClass)
UENUM(BlueprintType)
enum class EYearRange : uint8
{
    None = 0,
    Century1600 = 1,
    Century1700 = 2,
    Century1800 = 3,
    Decade1900 = 4,
    Decade1910 = 5,
    Decade1920 = 6,
    Decade1930 = 7,
    Decade1940 = 8,
    Decade1950 = 9,
    Decade1960 = 10,
    Decade1970 = 11,
    Decade1980 = 12,
    Decade1990 = 13,
    Decade2000 = 14,
    Decade2010 = 15,
    Decade2020 = 16,
    kNumYearRanges = 17
};
