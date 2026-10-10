#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HypeServerAuthority.h"
#include "FDSLobbyAuthority.generated.h"

class AHypeGameState;
class URequestMgr;
struct FFDSMainStageEvent;
class AHypePlayerController;
class AHypePlayerState;
class UMultiplayerGameplayState;
struct FMultiplayerGameStage;

// FDS's freestyle session authority: the server-side rules FUSER's native UMultiplayerGameplayState /
// UHypeMultiplayerServerBase code applied, reconstructed from the EGS v1.4.2 native bodies.
// - Lobby: players joining/leaving, teams, DJ indices, the DJ queue, readiness and leadership.
// - Set: the music clock, lobby completion with the measure layout, the stage driver and the server's stage
//   entry actions, and the return to the Lobby.
// What the fuserd heartbeat reports (FDSRegistration).
struct FFDSLobbyStatus
{
    int32 Players = 0;
    int32 DJs = 0;
    int32 Viewers = 0;
    int32 Stage = 0;        // EMultiplayerGameplayStage of the current stage
    int32 Round = 0;        // the timeline round (Lobby entries so far)
    int32 Rounds = 0;       // set length in use
    int32 TurnMeasures = 0;
    // Who is here, by hype user id (the Main Stage's admitted ids) with their team, the turn's DJ and the Main
    // Stage event being played.
    TArray<TPair<int32, EMultiplayerTeam>> Members;
    int32 CurrentDJ = 0;
    FString Event;

    bool SameLive(const FFDSLobbyStatus& Other) const
    {
        return Members == Other.Members && CurrentDJ == Other.CurrentDJ && Event == Other.Event;
    }
};

UCLASS()
class FUSERSERVER_API UFDSLobbyAuthority : public UObject, public IHypeServerAuthority
{
    GENERATED_BODY()

public:
    // The game state with its MultiplayerGameplayState and RequestMgr subobjects: HypeGameStateMultiplayerFreestyle, or
    // HypeGameStateMainStage with MainStageMPGameplayState in the Main Stage mode (FDSMainStage).
    void Start(AHypeGameState* InGameState, UMultiplayerGameplayState* InMps, URequestMgr* InRequestMgr, bool bInMainStage);
    void Stop();
    void Tick(float DeltaSeconds);
    void OnPlayerJoined(AHypePlayerState* Player);
    void OnPlayerLeft(AHypePlayerState* Player);

    // IHypeServerAuthority
    virtual void OnSetHypeUserId(AHypePlayerState* Player) override;
    virtual void OnSetIsReadyInLobby(AHypePlayerState* Player, bool bIsReady) override;
    virtual void OnSetIsReadyInResults(AHypePlayerState* Player, bool bIsReady) override;
    virtual void OnRequestTeamChange(AHypePlayerState* Player, EMultiplayerTeam NewTeam) override;
    virtual void OnMarkLobbyComplete(AHypePlayerState* Player) override;
    virtual void OnChangeLeaderTo(AHypePlayerState* Caller, AHypePlayerState* NewLeader) override;
    virtual void OnUpdateAfkSeconds(AHypePlayerController* Controller, int32 AfkSeconds) override;
    virtual void OnSendSignatureMove(AHypePlayerState* Player) override;
    virtual void OnLeaderKickPlayer(AHypePlayerState* Leader, AHypePlayerState* Target) override;
    virtual void OnToggleAfkChecks(AHypePlayerState* Player) override;
    virtual void OnRecordAction(AHypePlayerState* Player, FMultiplayerActionItemEntry& Item) override;
    virtual void OnRecordParam(AHypePlayerState* Player, const FMultiplayerUnreliableActionItemEntry& Item) override;
    virtual void OnAddCraftedInstrument(AHypePlayerState* Player, const FSerializableCraftedInstrumentCel& Data) override;
    virtual void OnRequestCraftedInstrument(AHypePlayerState* Player, FName ShortName) override;
    virtual void OnSendPraises(AHypePlayerState* Sender, const TArray<FCollectedPraise>& Praises) override;
    virtual void OnSendSnapshotTaken(AHypePlayerState* Sender, AHypePlayerState* ToPlayer, int32 AtMeasure) override;
    virtual void OnSubmitRequest(AHypePlayerState* Sender, const FMultiplayerLiveRequest& Request, AHypePlayerState* ToDJ) override;
    virtual void OnUpdateGlobalAudioState(AHypePlayerState* Player, int32 Tick, int32 BPM, EKey Key, EKeyMode Mode) override;
    virtual void OnSetCoopRoundNumTurnMeasures(AHypePlayerState* Player, int32 NumRounds, int32 TurnMeasures) override;

    FFDSLobbyStatus GetStatus() const;
    bool IsPublicSession() const { return ServerAccessState == EServerAccessState::PublicServer; }
    bool IsMainStage() const { return bMainStage; }

private:
    UMultiplayerGameplayState* Mps() const;
    TArray<AHypePlayerState*> Players(const AHypePlayerState* Except = nullptr) const;
    FMultiplayerGameStage* Stage(int32 Index) const;
    EMultiplayerGameplayStage CurrentStageType() const;
    float Now() const;

    // lobby
    void BuildStages();                                                      // FUN_14062f670
    void SetStage(int32 Index);                                              // FUN_140636bf0
    EMultiplayerTeam TeamForNewPlayer(const AHypePlayerState* Player) const; // FUN_140705120
    void SetTeam(AHypePlayerState* Player, EMultiplayerTeam Team);           // FUN_1406963e0
    void AssignDJIndices();                                                  // FUN_140636630
    void RebuildDJQueue();                                                   // FUN_140633500
    void UpdateEveryoneReady();                                              // FUN_1406383b0
    void SetSessionLeader(AHypePlayerState* Leader);                         // FUN_1406965a0
    void LogPlayers(const TCHAR* Why) const;

    // music clock (the authority's audio timeline in FUSER; a software clock here)
    int32 ClockTick() const;                     // FUN_1405928e0(clock, 0): the raw master tick, -1 when stopped
    int32 ClockMeasure() const;                  // FUN_140592780
    float SecondsUntilTick(int32 TargetTick) const; // FUN_1405a84c0 less the current time, at the current BPM
    void RestartClock(int32 Bpm);                // FUN_14059def0
    void SetClockBpm(int32 Bpm);                 // FUN_1405a1eb0
    void UpdateTimelineState();                  // FUN_140653660 (MusicTimelineState authority tick)

    // set
    bool CompleteLobby(const TCHAR* Why);                       // FUN_14062bac0
    void UpdatePublicLobbyTimer();                              // MPS slot 146, FUN_140637a40
    void LayOutStages(int32 StartMeasure);                      // FUN_14062ef40
    int32 BarsFor(EMultiplayerGameplayStage Type) const;        // MPS slot 124, FUN_1406241f0
    AHypePlayerState* PlayerForDJIndex(int32 DJIndex) const;    // FUN_140632360
    bool StageCounts(const FMultiplayerGameStage& S) const;     // the DJ / measure-count test in FUN_140632050
    int32 StageForMeasure(int32 Measure) const;                 // FUN_140632050 (MPS slot +0x430)
    void DriveStages();
    void ShuffleDJOrder();                                      // MPS slot +0x498, FUN_140637660
    void PlaceResultsWithoutHottestMix();                       // FUN_140636bf0 case RoundComplete_FarShot
    void CollapseHottestMix(const TCHAR* Why);                  // FUN_140636bf0 case RoundComplete_FarShot, no winner
    void ShiftStagesFrom(int32 Index, int32 Bars);              // FUN_140636bf0 tail
    void DropPerformer(AHypePlayerState* Player);               // a DJ left mid-set: their turn ends early
    FString StageLabel(int32 Index) const;
    void SeedStateOfTheWorld(AHypePlayerState* Player);         // the dedicated server's late-join snapshot
    void AssignPendingTeams();
    void RestoreDJsAfterSet();
    void PruneActions();
    void ClearActions(const TCHAR* Why);

    double ClockAnchorSeconds = 0.0;
    int32 ClockAnchorTick = 0;
    int32 ClockBpm = 128;
    bool bClockRunning = false;
    int32 SyncTick = 0;              // the song anchor (ServerDefaultSyncTick)
    int32 RoundIndex = 0;            // _syncState.roundIndex: Lobby entries so far
    int32 LastLoggedBpm = -1;
    EKey MasterKey = EKey::C;        // the DJ's latest key / mode (ServerUpdateGlobalAudioState)
    EKeyMode MasterMode = EKeyMode::Major;
    int32 NextActionId = 1;          // UniqueId of server-created action items

    EServerAccessState ServerAccessState = EServerAccessState::PrivateServer;
    TWeakObjectPtr<AHypeGameState> GameState;
    TWeakObjectPtr<UMultiplayerGameplayState> MpsObject;
    TWeakObjectPtr<URequestMgr> RequestMgrObject;
    // The Main Stage mode: no ready-up, leader, lobby timer or set-length choice. The event schedule picks
    // the DJs and starts the sets (UpdateMainStage).
    bool bMainStage = false;
    FString StartedEvent;                                          // the event whose set was started (or skipped)
    FString NoHeadlinersLogged;                                    // the event whose "nobody here yet" was logged
    float LateFirstDJAt = -1.0f;                                   // world time a late event's first DJ was placed
    const FFDSMainStageEvent* PlacementEvent(bool* bLate = nullptr) const; // the event whose headliners DJ now
    void PruneDeparted();                                          // drop departed players from the server's maps
    float NextMainStageUpdate = 0.0f;
    void UpdateMainStage();                                        // MPS slots 145 / 148 / 149, server side
    EMultiplayerTeam MainStageTeam(const AHypePlayerState* Player) const; // slot 148: a next-event headliner is a DJ
    int32 HeadlinerRank(const AHypePlayerState* Player) const;     // slot 147: the position in the headliner list

    // The Main Stage crowd. Reactions are tallied per praise type and turned into per-second rates
    // (MainStageRhythmojiRates) every ServerMainStageRhythmojiProcessingTime; clients simulate the crowd from them.
    TArray<int32> RhythmojiCounts;
    float RhythmojiWindowStart = -1.0f;
    float RhythmojiProcessingTime = 1.0f;  // HypeMultiplayerSettings +0x2f8 ServerMainStageRhythmojiProcessingTime
    bool bMainStageMulticastPraises = false;
    void UpdateRhythmojiRates();
    void PublishMainStageResults();        // ResultsState._mainStageResults, as the Main Stage client reads them
    TWeakObjectPtr<AHypePlayerState> Leaving;
    TWeakObjectPtr<AHypePlayerState> CurrentDJ;
    TMap<TWeakObjectPtr<AHypePlayerState>, float> PendingTeam; // admitted, awaiting ServerSetHypeUserId (world time)
    static constexpr float TeamFallbackSeconds = 2.0f;
    TArray<TWeakObjectPtr<AHypePlayerState>> DJsToRestore;      // lobby DJ refresh after a set
    float DJRestoreAt = 0.0f;
    static constexpr float DJRefreshSeconds = 1.0f;
    TMap<int32, float> ActionRecordedAt;                        // UniqueId -> world time recorded (pruning)
    TMap<FString, int32> ActionCounts;                          // per action type, this set
    float NextPruneAt = 0.0f;
    TSet<TWeakObjectPtr<AHypePlayerState>> HadTurn;             // idle rule: PS +0x1110
    TSet<TWeakObjectPtr<AHypePlayerState>> Acted;               // idle rule: PS +0x1111
    TMap<FName, FSerializableCraftedInstrumentCel> CraftedRegistry;            // GameState +0x448 in FUSER
    TMap<TWeakObjectPtr<AHypePlayerState>, TArray<FName>> PendingCrafted;      // PS +0x1100 in FUSER
    void ServeCraftedRequests();                                               // FUN_14069f770

    // results
    void ResetResults(AHypePlayerState* Player);                              // FUN_140651ba0 / FUN_14064f330
    bool CountAtMeasure(AHypePlayerState* Owner, TArray<FPerMeasureResultSingleRoundStat>& Table, int32 Measure); // FUN_14065d0b0
    void UpdateFans(AHypePlayerState* Target);                                 // FUN_14065d190
    // ResultsState's non-replicated server fields: _serverPraiseCountsTowardsOtherPlayers / _serverMostPraisedPlayer
    TMap<TWeakObjectPtr<AHypePlayerState>, TMap<TWeakObjectPtr<AHypePlayerState>, int32>> PraiseGiven;
    TMap<TWeakObjectPtr<AHypePlayerState>, TPair<TWeakObjectPtr<AHypePlayerState>, int32>> MostPraised;

    // live requests
    TArray<FMultiplayerLiveRequest>* LiveRequests() const;
    void ScheduleRequests();                                  // the compiled-out scheduler
    void CompleteRequest(const FMultiplayerActionItemEntry& Item, AHypePlayerState* DJ);
    void ClearRequests(const TCHAR* Why);
    int32 NextRequestId = 0;                                  // RequestMgr +0x1b0

    // The current mix and the per-measure mix history: what FUSER's dedicated server knew by playing the
    // DJs' actions. Each category holds the last action item that set it (whole copies, reused for a late joiner's
    // state of the world).
    struct FFDSMix
    {
        TMap<uint8, FMultiplayerActionItemEntry> Playing;     // by deck track: the SpawnDisc (as Play) now playing
        TMap<uint8, FMultiplayerActionItemEntry> Cued;        // by deck track: the SpawnDisc (as Queue) waiting
        TMap<uint8, FMultiplayerActionItemEntry> Gain;        // by track: the last TrackGain
        TMap<uint8, FMultiplayerActionItemEntry> Effect;      // by effect slot: the last EffectSlotState
        TMap<uint8, FMultiplayerActionItemEntry> Instrument;  // by crafting slot: the current crafted instrument
        TOptional<FMultiplayerActionItemEntry> Global;        // the last GlobalAudioState
        TOptional<FMultiplayerActionItemEntry> SoloMute;      // the last SoloMuteStates
        int32 Tick = -1;                                      // the newest folded stamp
        int32 Actions = 0;
    };
    FFDSMix Mix;
    bool bMixDirty = false;
    TMap<int32, FMultiplayerMeasureHistoryEntry> MixHistory;  // master measure -> entry (MPS +0x390 in FUSER)
    int32 LastHistoryMeasure = -1;
    void FoldIntoMix(const FMultiplayerActionItemEntry& Item);
    void ResetMix(const TCHAR* Why);
    FMultiplayerMeasureHistoryEntry MakeHistoryEntry() const;
    void RecordMixHistory();
    bool ScoreHottestMix();                                   // FUN_140636bf0 case FarShot: FUN_140651a10 per player
    int32 CurrentStage = -1;
    int32 SetStartMeasure = -1;      // MPS +0x368
    bool bLobbyTimerRunning = false; // the Lobby's end time is the public lobby timer's, not a ready countdown's
    float AutoReadyAt = -1.0f;       // -FDSAutoReadyTest: world time the queued DJs are readied
    TArray<TWeakObjectPtr<AHypePlayerState>> DJQueue;
    TArray<TWeakObjectPtr<AHypePlayerState>> Spectators;
    bool bEveryoneReady = false;
    int32 ReadyCount = 0;

    // [/Script/FuserServer.FDSLobby] in Game.ini; defaults as FUSER's HypeMultiplayerSettings.
    int32 CoopRounds = 3;
    int32 CoopTurnMeasures = 128;
    int32 MaxNumDJs = 4;
    bool bAutoStart = true;
    bool bCollapseHottestMixWithoutWinner = true;  // FUSER's own behaviour when nobody scored (see the .cpp)
    bool bRefreshDJsAfterSet = true;               // lobby DJ refresh after a set (see SetStage, Lobby)
    bool bResultsXPPayout = true;                  // run Results_XPPayout after Results (see DriveStages)
    bool bSkipHottestMixWhenSolo = true;           // a one-player set skips the hottest-mix stages (see SetStage)
    bool bSetLengthChosen = false;                 // the first player's ServerSetCoopRoundNumTurnMeasures was taken
    bool bPublicLobbyTimer = true;                 // a public session's Lobby counts down by itself (UpdatePublicLobbyTimer)
    float PublicLobbyTimerSeconds = 60.0f;         // PerStageRealtimeSeconds[Lobby]
    int32 PublicLobbyTimerMinDJs = 2;              // FUSER's rule: the timer needs two DJs in the queue
    // FUSER HypeMultiplayerSettings AFK keys, same names and format: idle seconds before a kick, keyed by the co-op
    // turn length in measures, else the default.
    TMap<int32, int32> AfkSecondsTillKick = {{32, 600}, {64, 900}, {128, 1200}};
    int32 AfkSecondsTillKickDefault = 600;
    bool bEnableAfkChecks = true;
};
