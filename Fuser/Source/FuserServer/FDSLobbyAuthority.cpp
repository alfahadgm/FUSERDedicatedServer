#include "FDSLobbyAuthority.h"
#include "FuserServer.h"
#include "DJPlayerState.h"
#include "FDSMainStage.h"
#include "FDSRegistration.h"
#include "HypeGameState.h"
#include "HypePlayerState.h"
#include "MultiplayerGameplayState.h"
#include "MusicTimelineState.h"
#include "ResultsState.h"
#include "RequestMgr.h"
#include "HypePlayerController.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"

namespace
{
    constexpr uint8 ReadyLobbyBit = 1;   // EHypePlayerStateIsReadyFlags::Lobby
    constexpr uint8 ReadyResultsBit = 2; // EHypePlayerStateIsReadyFlags::Results

    uint8 ReadyBits(const AHypePlayerState* Player) { return static_cast<uint8>(Player->_isReadyCheck); }

    const TCHAR* TeamName(EMultiplayerTeam Team)
    {
        return Team == EMultiplayerTeam::DJ ? TEXT("DJ") : Team == EMultiplayerTeam::Spectator ? TEXT("Spectator") : TEXT("Unknown");
    }

    // FUSER DefaultGame.ini [/Script/Hype.HypeMultiplayerSettings] (offsets from FUSER's reflection data).
    // PerStageRealtimeSeconds[stage type]: a FUSER host's lobby shows it in each stage's
    // RoundRealtimeSeconds (Lobby 60, XP payout 30).
    constexpr float PerStageRealtimeSeconds[] = {-1.f, 60.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f,
                                                 30.f, -1.f, 8.f, 3.f, -1.f, 2.5f, 3.3f, 8.f, 5.f, -1.f};
    // PerStageMeasureCount[stage type] (+0x15c): bars per stage in the set layout; Playing uses the turn length.
    constexpr int32 PerStageMeasureCount[] = {-1, -1, 1, 1, 1, 2, 32, 2, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    constexpr int32 ClientNumMeasuresBehind = 1;  // +0xa0: listeners run this many bars behind the master clock
    constexpr float MinLobbyReadyTimer = 5.0f;     // +0xd8: shortest lobby countdown, seconds
    constexpr int32 MaxLobbyReadyTimerMeasures = 7; // +0xdc: furthest start bar, in bars from now
    constexpr int32 MainStageXPResultsBars = 12;    // +0x88 NumMeasuresForMainStageXPResults
    constexpr int32 TurnBlockBars = 32;             // Main Stage turns are whole 32-bar blocks
    constexpr int32 MainStageMaxTurnBars = 640;     // +0x84 TotalMeasuresForMainStageDJPerformances, as a turn's limit

    // Music timeline resolution: 480 ticks per beat, 4 beats per bar (DAT_1429c84c8 = 1920); BPM x 8 ticks per second
    // (a FUSER host's authority raw clock: 1023.97 ticks/s at 128 BPM).
    constexpr int32 TicksPerBeat = 480;
    constexpr int32 TicksPerBar = 1920;
    constexpr int32 LobbyBPM = 128;

    // One stage record as FUN_14062f670 writes it: everything "unset" (-1 / 128 BPM) apart from type, screen, DJ, round.
    FMultiplayerGameStage MakeStage(EMultiplayerGameplayStage Type, EShellScreenType Screen, int32 DJIndex = -1, int32 Round = -1)
    {
        FMultiplayerGameStage Stage;
        Stage.Stage = Type;
        Stage.ShellScreen = Screen;
        Stage.MainStageSpectatorAltShellScreen = EShellScreenType::None;
        const int32 TypeIndex = static_cast<int32>(Type);
        Stage.RoundRealtimeSeconds = TypeIndex < UE_ARRAY_COUNT(PerStageRealtimeSeconds) ? PerStageRealtimeSeconds[TypeIndex] : -1.0f;
        Stage.ServerHasCompletedStage = false;
        Stage.ServerWorldTimeStageEnd = -1.0f;
        Stage.djIndex = DJIndex;
        Stage.RoundNumber = Round;
        Stage.DJForStageExistsOnServer = false;
        Stage.startMeasure = -1;
        Stage.endMeasure = -1;
        Stage.StageWinner = EBattlePlayerLabel::None;
        Stage.BattleRoundIdx = -1;
        Stage.BattleBPM = 128;
        Stage.BattleKey = EKey::C;
        Stage.BattleKeyMode = EKeyMode::Major;
        return Stage;
    }
}

void UFDSLobbyAuthority::Start(AHypeGameState* InGameState, UMultiplayerGameplayState* InMps, URequestMgr* InRequestMgr, bool bInMainStage)
{
    GameState = InGameState;
    MpsObject = InMps;
    RequestMgrObject = InRequestMgr;
    bMainStage = bInMainStage;
    GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("CoopRounds"), CoopRounds, GGameIni);
    GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("CoopTurnMeasures"), CoopTurnMeasures, GGameIni);
    GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("MaxNumDJsInSession"), MaxNumDJs, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("AutoStart"), bAutoStart, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("CollapseHottestMixWithoutWinner"), bCollapseHottestMixWithoutWinner, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("RefreshDJsAfterSet"), bRefreshDJsAfterSet, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("ResultsXPPayout"), bResultsXPPayout, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("SkipHottestMixWhenSolo"), bSkipHottestMixWhenSolo, GGameIni);
    FString Access = TEXT("Private");
    GConfig->GetString(TEXT("/Script/FuserServer.FDSLobby"), TEXT("ServerAccessState"), Access, GGameIni);
    // A container fuserd starts gets the session's set length and access in its
    // environment: FDS_ROUNDS / FDS_TURN_MEASURES (the ticket's choice; the first player's
    // ServerSetCoopRoundNumTurnMeasures can still replace it) and FDS_SERVER_ACCESS (public / private).
    const int32 EnvRounds = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_ROUNDS")));
    const int32 EnvMeasures = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_TURN_MEASURES")));
    if (EnvRounds > 0 && EnvRounds <= 10 && EnvMeasures >= 8 && EnvMeasures <= 512)
    {
        CoopRounds = EnvRounds;
        CoopTurnMeasures = EnvMeasures;
    }
    const FString EnvAccess = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_SERVER_ACCESS"));
    if (EnvAccess.Equals(TEXT("public"), ESearchCase::IgnoreCase) || EnvAccess.Equals(TEXT("private"), ESearchCase::IgnoreCase))
    {
        Access = EnvAccess.Equals(TEXT("public"), ESearchCase::IgnoreCase) ? TEXT("Public") : TEXT("Private");
    }
    ServerAccessState = Access == TEXT("Public") ? EServerAccessState::PublicServer : EServerAccessState::PrivateServer;
    UE_LOG(LogFDS, Display, TEXT("FDS: set length %d round(s) of %d measures%s; %s session"), CoopRounds, CoopTurnMeasures,
        EnvRounds > 0 ? TEXT(" (FDS_ROUNDS / FDS_TURN_MEASURES)") : TEXT(""), *Access);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("PublicLobbyTimer"), bPublicLobbyTimer, GGameIni);
    GConfig->GetFloat(TEXT("/Script/FuserServer.FDSLobby"), TEXT("PublicLobbyTimerSeconds"), PublicLobbyTimerSeconds, GGameIni);
    GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("PublicLobbyTimerMinDJs"), PublicLobbyTimerMinDJs, GGameIni);
    PublicLobbyTimerSeconds = FMath::Max(PublicLobbyTimerSeconds, MinLobbyReadyTimer);
    PublicLobbyTimerMinDJs = FMath::Max(PublicLobbyTimerMinDJs, 1);
    UE_LOG(LogFDS, Display, TEXT("FDS: public lobby timer %s (%.0f s from %d DJs)%s"), bPublicLobbyTimer ? TEXT("on") : TEXT("off"),
        PublicLobbyTimerSeconds, PublicLobbyTimerMinDJs, bPublicLobbyTimer && !IsPublicSession() ? TEXT("; not used: private session") : TEXT(""));
    FString AfkTable;
    if (GConfig->GetString(TEXT("/Script/FuserServer.FDSLobby"), TEXT("AfkSecondsTillKick"), AfkTable, GGameIni))
    {
        // "((32, 600),(64, 900),...)": the numbers in order are measure/seconds pairs. Parsed by hand: FRegexMatcher
        // aborts in the packaged server (no ICU data staged).
        TArray<int32> Numbers;
        for (int32 i = 0; i < AfkTable.Len(); ++i)
        {
            if (FChar::IsDigit(AfkTable[i]))
            {
                int32 Value = 0;
                for (; i < AfkTable.Len() && FChar::IsDigit(AfkTable[i]); ++i)
                {
                    Value = Value * 10 + (AfkTable[i] - TEXT('0'));
                }
                Numbers.Add(Value);
            }
        }
        AfkSecondsTillKick.Reset();
        for (int32 i = 0; i + 1 < Numbers.Num(); i += 2)
        {
            AfkSecondsTillKick.Add(Numbers[i], Numbers[i + 1]);
        }
    }
    GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("AfkSecondsTillKickDefault"), AfkSecondsTillKickDefault, GGameIni);
    GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("EnableAfkChecks"), bEnableAfkChecks, GGameIni);
    if (bMainStage)
    {
        // The Main Stage: a public venue nobody readies or leads; sets follow the event schedule. FUSER's
        // Main Stage MPS has no timed lobby start (slot 146 is the empty stub), and its AFK threshold is
        // AfkSecondsTillKickMainStageSpectator / Performer (both 1800 s in the shipping settings) whatever the turn length.
        bAutoStart = false;
        bPublicLobbyTimer = false;
        bRefreshDJsAfterSet = false; // the next event's headliners are placed by UpdateMainStage
        MaxNumDJs = 4;               // the event's slots (4); set per event before its set starts
        ServerAccessState = EServerAccessState::PublicServer;
        AfkSecondsTillKick.Reset();
        AfkSecondsTillKickDefault = 1800;
        GConfig->GetFloat(TEXT("/Script/FuserServer.FDSLobby"), TEXT("MainStageRhythmojiProcessingTime"), RhythmojiProcessingTime, GGameIni);
        GConfig->GetBool(TEXT("/Script/FuserServer.FDSLobby"), TEXT("MainStageMulticastPraises"), bMainStageMulticastPraises, GGameIni);
        RhythmojiProcessingTime = FMath::Max(RhythmojiProcessingTime, 0.25f);
        GConfig->GetInt(TEXT("/Script/FuserServer.FDSLobby"), TEXT("AfkSecondsTillKickMainStage"), AfkSecondsTillKickDefault, GGameIni);
        UE_LOG(LogFDS, Display, TEXT("FDS: Main Stage mode: the schedule's headliners DJ, everyone else spectates; no auto-start, lobby timer or leader; AFK kick after %d s"),
            AfkSecondsTillKickDefault);
    }
    FString AfkText;
    for (const TPair<int32, int32>& Entry : AfkSecondsTillKick)
    {
        AfkText += FString::Printf(TEXT("%s%d measures: %d s"), AfkText.IsEmpty() ? TEXT("") : TEXT(", "), Entry.Key, Entry.Value);
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: AFK checks %s; kick after %s; default %d s"), bEnableAfkChecks ? TEXT("on") : TEXT("off"), *AfkText,
        AfkSecondsTillKickDefault);
    IHypeServerAuthority::Register(GetWorld(), this);
    if (UMultiplayerGameplayState* M = Mps())
    {
        M->_numCoopFreestyleRounds = CoopRounds;
        M->_coopTurnLengthMeasures = CoopTurnMeasures;
        // FUSER's set stage does nothing with an empty list: build it (slot 130), then enter the Lobby (slot 131).
        BuildStages();
        SetStage(0);
    }
    else
    {
        UE_LOG(LogFDS, Error, TEXT("FDS: lobby authority: game state has no MultiplayerGameplayState"));
    }
}

void UFDSLobbyAuthority::Stop()
{
    IHypeServerAuthority::Unregister(GetWorld(), this);
}

// Run from the game mode's tick: the authority's MPS and MusicTimelineState ticks (FUN_14062dec0, FUN_140653660)
// plus the stage driver the dedicated build compiled out.
void UFDSLobbyAuthority::Tick(float DeltaSeconds)
{
    if (!Mps())
    {
        return;
    }
    // Test hook: -FDSAutoReadyTest readies every queued DJ in the Lobby, so a test client that
    // cannot use the lobby UI still runs a whole set; -FDSAutoReadyTestDelay=<s> waits that long after the first DJ is
    // queued (the public lobby timer tests). Never set in normal runs.
    static const bool bAutoReadyTest = FParse::Param(FCommandLine::Get(), TEXT("FDSAutoReadyTest"));
    static const float AutoReadyDelay = []
    {
        float Delay = 0.0f;
        FParse::Value(FCommandLine::Get(), TEXT("FDSAutoReadyTestDelay="), Delay);
        return Delay;
    }();
    if (bAutoReadyTest && CurrentStageType() == EMultiplayerGameplayStage::Lobby && !bEveryoneReady && DJQueue.Num() > 0)
    {
        if (AutoReadyAt < 0.0f)
        {
            AutoReadyAt = Now() + AutoReadyDelay;
        }
        if (Now() >= AutoReadyAt)
        {
            for (const TWeakObjectPtr<AHypePlayerState>& Player : DJQueue)
            {
                if (Player.IsValid() && !(ReadyBits(Player.Get()) & ReadyLobbyBit))
                {
                    Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(ReadyBits(Player.Get()) | ReadyLobbyBit);
                    UE_LOG(LogFDS, Display, TEXT("FDS: test: player %d readied (-FDSAutoReadyTest)"), Player->PlayerId);
                }
            }
            UpdateEveryoneReady();
        }
    }
    else if (CurrentStageType() != EMultiplayerGameplayStage::Lobby)
    {
        AutoReadyAt = -1.0f;
    }
    AssignPendingTeams();
    RestoreDJsAfterSet();
    PruneActions();
    UpdateTimelineState();
    RecordMixHistory();
    UpdatePublicLobbyTimer();
    UpdateMainStage();
    UpdateRhythmojiRates();
    DriveStages();
}

UMultiplayerGameplayState* UFDSLobbyAuthority::Mps() const
{
    return MpsObject.Get();
}

FMultiplayerGameStage* UFDSLobbyAuthority::Stage(int32 Index) const
{
    UMultiplayerGameplayState* M = Mps();
    return M && M->_gameStages.IsValidIndex(Index) ? &M->_gameStages[Index] : nullptr;
}

// Server world time (seconds): what FUSER's stage end times and _syncState.ServerWorldSeconds use.
float UFDSLobbyAuthority::Now() const
{
    return GameState.IsValid() ? GameState->GetServerWorldTimeSeconds() : 0.0f;
}

FString UFDSLobbyAuthority::StageLabel(int32 Index) const
{
    const FMultiplayerGameStage* S = Stage(Index);
    return FString::Printf(TEXT("%s (%d)"), S ? *StaticEnum<EMultiplayerGameplayStage>()->GetNameStringByValue(static_cast<int64>(S->Stage)) : TEXT("none"), Index);
}

TArray<AHypePlayerState*> UFDSLobbyAuthority::Players(const AHypePlayerState* Except) const
{
    TArray<AHypePlayerState*> Out;
    if (GameState.IsValid())
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            AHypePlayerState* Player = Cast<AHypePlayerState>(PS);
            if (Player && Player != Except && Player != Leaving.Get() && !Player->IsPendingKill())
            {
                Out.Add(Player);
            }
        }
    }
    return Out;
}

EMultiplayerGameplayStage UFDSLobbyAuthority::CurrentStageType() const
{
    const UMultiplayerGameplayState* M = Mps();
    return M && M->_gameStages.IsValidIndex(CurrentStage) ? M->_gameStages[CurrentStage].Stage : EMultiplayerGameplayStage::None;
}

// FUN_14062f670: Lobby, the two round-start stages, three stages per DJ slot per round, the round-complete stages
// and the results. The DJ-slot count is MPS vtable +0x3f8 (MaxNumDJsInSession; a FUSER host built 44 stages for 3 rounds).
void UFDSLobbyAuthority::BuildStages()
{
    UMultiplayerGameplayState* M = Mps();
    TArray<FMultiplayerGameStage>& Stages = M->_gameStages;
    Stages.Reset();
    Stages.Add(MakeStage(EMultiplayerGameplayStage::Lobby, EShellScreenType::MultiplayerLobby));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::RoundStart_SetupGameplay, EShellScreenType::Gameplay));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::RoundStart_DJOrderIntro, EShellScreenType::Gameplay));
    for (int32 Round = 0; Round < M->_numCoopFreestyleRounds; ++Round)
    {
        for (int32 DJ = 0; DJ < MaxNumDJs; ++DJ)
        {
            Stages.Add(MakeStage(EMultiplayerGameplayStage::LoadStageKit, EShellScreenType::Gameplay, DJ, Round));
            Stages.Add(MakeStage(EMultiplayerGameplayStage::UpcomingDJ, EShellScreenType::Gameplay, DJ, Round));
            Stages.Add(MakeStage(EMultiplayerGameplayStage::Playing, EShellScreenType::Gameplay, DJ, Round));
        }
    }
    Stages.Add(MakeStage(EMultiplayerGameplayStage::RoundComplete_FarShot, EShellScreenType::Gameplay));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::RoundComplete_HottestMixIntro, EShellScreenType::Gameplay));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::RoundComplete_HottestMixPlayback, EShellScreenType::Gameplay));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::Results, EShellScreenType::MultiplayerResults));
    Stages.Add(MakeStage(EMultiplayerGameplayStage::Results_XPPayout, EShellScreenType::MultiplayerResults));
    // FUSER then calls MPS vtable +0x458 and FUN_14062f480; a FUSER host's lobby shows only the realtime seconds above.
}

// ---------------------------------------------------------------------------------------------- music clock
// FUSER's authority clock was its audio engine's timeline; FDS keeps a software one: the master tick runs at
// BPM x 8 ticks per second from an anchor, re-anchored on every tempo change, in server world time.

int32 UFDSLobbyAuthority::ClockTick() const
{
    if (!bClockRunning)
    {
        return -1;
    }
    const double Elapsed = FMath::Max(0.0, static_cast<double>(Now()) - ClockAnchorSeconds);
    return ClockAnchorTick + static_cast<int32>(FMath::FloorToDouble(Elapsed * ClockBpm * (TicksPerBeat / 60.0)));
}

int32 UFDSLobbyAuthority::ClockMeasure() const
{
    const int32 Tick = ClockTick();
    return Tick < 0 ? -1 : Tick / TicksPerBar;
}

float UFDSLobbyAuthority::SecondsUntilTick(int32 TargetTick) const
{
    return static_cast<float>((TargetTick - ClockTick()) / (ClockBpm * (TicksPerBeat / 60.0)));
}

// FUN_14059def0 (Lobby entry): the timeline restarts from tick 0. Clients see the master tick go backwards
// (OnRep_SyncState broadcasts OnServerTimelineLooped: the lobby entrance).
void UFDSLobbyAuthority::RestartClock(int32 Bpm)
{
    ClockAnchorSeconds = Now();
    ClockAnchorTick = 0;
    ClockBpm = Bpm;
    bClockRunning = true;
    UE_LOG(LogFDS, Display, TEXT("FDS: music clock restarted at world %.2f s (%d BPM, %d ticks per beat)"), ClockAnchorSeconds, ClockBpm, TicksPerBeat);
}

void UFDSLobbyAuthority::SetClockBpm(int32 Bpm)
{
    ClockAnchorTick = ClockTick();
    ClockAnchorSeconds = Now();
    ClockBpm = Bpm;
}

// FUN_140653660, authority branch, every tick: world seconds, the raw master tick, the master BPM and the song anchor.
// roundIndex is counted by Lobby entries (FUN_140651b90): 1 in the first lobby, as on a FUSER host.
void UFDSLobbyAuthority::UpdateTimelineState()
{
    UMusicTimelineState* Timeline = GameState.IsValid() ? GameState->MusicTimelineState : nullptr;
    if (!Timeline || !bClockRunning)
    {
        return;
    }
    FReplicatedTimelineSynchronizationState& Sync = Timeline->_syncState;
    Sync.roundIndex = RoundIndex;
    Sync.ServerWorldSeconds = Now();
    Sync.ServerMasterTick = ClockTick();
    Sync.ServerBPM = ClockBpm;
    Sync.ServerDefaultSyncTick = SyncTick;
}

// ---------------------------------------------------------------------------------------------- stages

// FUN_140636bf0: the server's stage entry. Earlier stages are marked completed, then the per-type entry actions;
// entering SetupGameplay or RoundComplete_FarShot above 120 BPM moves the rest of the set one bar later.
void UFDSLobbyAuthority::SetStage(int32 Index)
{
    UMultiplayerGameplayState* M = Mps();
    const FString Previous = StageLabel(CurrentStage);
    const int32 PreviousIndex = CurrentStage;
    if (CurrentStageType() == EMultiplayerGameplayStage::Lobby)
    {
        for (AHypePlayerState* Player : Players())
        {
            Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(ReadyBits(Player) & ~ReadyLobbyBit);
        }
    }
    if (CurrentStageType() == EMultiplayerGameplayStage::Playing)
    {
        ClearRequests(TEXT("turn over"));
    }
    if (CurrentStageType() == EMultiplayerGameplayStage::Results_XPPayout)
    {
        // FUN_140636bf0, leaving Results_XPPayout: ServerSetIsReadyInResults(false) on every player.
        for (AHypePlayerState* Player : Players())
        {
            Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(ReadyBits(Player) & ~ReadyResultsBit);
            Player->ForceNetUpdate();
        }
    }
    CurrentStage = Index;
    for (int32 i = 0; i < M->_gameStages.Num(); ++i)
    {
        M->_gameStages[i].ServerHasCompletedStage = i < CurrentStage;
    }
    const EMultiplayerGameplayStage Type = CurrentStageType();
    CurrentDJ = nullptr; // FUN_140638040: only Playing has a current DJ
    switch (Type)
    {
    case EMultiplayerGameplayStage::Lobby:
        M->_hottestMix = FMultiplayerMeasureHistoryEntry();
        M->_hottestMix.BPM = 128;
        ResetMix(TEXT("lobby"));
        MixHistory.Reset();
        LastHistoryMeasure = -1;
        BuildStages();
        for (AHypePlayerState* Player : Players())
        {
            ResetResults(Player); // FUN_140636bf0 case Lobby: FUN_140651ba0(PS +0x10c0, true) for every player
        }
        SetStartMeasure = -1; // before AssignDJIndices, which lays the stages out again from a set start bar
        bLobbyTimerRunning = false;
        AssignDJIndices();
        RebuildDJQueue();
        // The clock restarts (FUN_14059def0, which keeps the tempo) with the song anchor 0, and the timeline's
        // round counts up (FUN_140651b90). Before the first player FDS leaves the clock stopped: a FUSER
        // host's lobby master tick counts from the first arrival.
        if (bClockRunning)
        {
            RestartClock(ClockBpm);
        }
        SyncTick = 0;
        ++RoundIndex;
        if (PreviousIndex > 0)
        {
            ClearActions(TEXT("back in the Lobby"));
            // FUSER's idle rule (HypePlayerState::_OnMultiplayerGameplayStateChanged, FUN_14069ea10, server only): with
            // AFK checks on, a DJ who had a turn (+0x1110, set entering Playing) and never acted (+0x1111, set by the
            // compiled-out action bodies; here: any recorded action during the set) becomes a Spectator.
            if (bEnableAfkChecks)
            {
                for (const TWeakObjectPtr<AHypePlayerState>& Player : HadTurn)
                {
                    if (Player.IsValid() && !Acted.Contains(Player) && Player->_team == EMultiplayerTeam::DJ)
                    {
                        UE_LOG(LogFDS, Display, TEXT("FDS: player %d had a turn without acting: moved to viewers (FUSER idle rule)"), Player->PlayerId);
                        SetTeam(Player.Get(), EMultiplayerTeam::Spectator);
                    }
                }
                AssignDJIndices();
                RebuildDJQueue();
            }
            HadTurn.Reset();
            Acted.Reset();
        }
        // Back from a set: the clients' lobby DJ characters only reappear on a DJ queue change (seen
        // missing after the return, back as soon as a player left). FUSER's own return changed the queue when its idle
        // rule demoted DJs (in a shipping build all of them: nothing sets "acted"). For the DJs still on the team, the DJs are moved to Spectator and restored after DJRefreshSeconds
        // in their order, so every client rebuilds its queue. RefreshDJsAfterSet=False turns it off.
        if (bRefreshDJsAfterSet && PreviousIndex > 0)
        {
            DJsToRestore.Reset();
            for (const TWeakObjectPtr<AHypePlayerState>& Player : DJQueue)
            {
                if (Player.IsValid())
                {
                    DJsToRestore.Add(Player);
                    SetTeam(Player.Get(), EMultiplayerTeam::Spectator);
                }
            }
            AssignDJIndices();
            RebuildDJQueue();
            DJRestoreAt = Now() + DJRefreshSeconds;
            UE_LOG(LogFDS, Display, TEXT("FDS: back from the set: %d DJ(s) moved to viewers, restored in %.1f s (lobby DJ refresh)"), DJsToRestore.Num(),
                DJRefreshSeconds);
        }
        UpdateEveryoneReady();
        break;
    case EMultiplayerGameplayStage::RoundStart_SetupGameplay:
        if (!bMainStage) // the Main Stage keeps the headliner order (MPS slot 147 replaces the shuffle)
        {
            ShuffleDJOrder();
        }
        RebuildDJQueue();
        break;
    case EMultiplayerGameplayStage::Playing:
        // FUN_140638040: the turn's DJ. The song anchor moves to the turn's first tick, so each turn's songs start from
        // bar 1 on every client.
        CurrentDJ = PlayerForDJIndex(Stage(CurrentStage)->djIndex);
        if (CurrentDJ.IsValid())
        {
            HadTurn.Add(CurrentDJ); // FUN_14069ea10: PS +0x1110
        }
        if (Stage(CurrentStage)->startMeasure >= 0)
        {
            SyncTick = Stage(CurrentStage)->startMeasure * TicksPerBar;
        }
        break;
    case EMultiplayerGameplayStage::RoundComplete_FarShot:
        if (bSkipHottestMixWhenSolo && GetStatus().Players <= 1)
        {
            // SkipHottestMixWhenSolo: a solo player goes straight to Results; their mix plays on into the Lobby.
            CollapseHottestMix(TEXT("only one player"));
        }
        else if (!ScoreHottestMix())
        {
            PlaceResultsWithoutHottestMix();
        }
        break;
    case EMultiplayerGameplayStage::Results:
        if (bMainStage)
        {
            PublishMainStageResults();
        }
        break;
    case EMultiplayerGameplayStage::RoundComplete_HottestMixPlayback:
        ResetMix(TEXT("hottest mix playback: the clients clear their decks"));
        break;
    case EMultiplayerGameplayStage::Results_XPPayout:
    {
        // FUN_140636bf0 case Results_XPPayout: the stage ends RoundRealtimeSeconds (30) from now. On the Main Stage it is
        // also laid out in bars (BarsFor: 12), which end it sooner; the client counts down to its end bar.
        FMultiplayerGameStage* Payout = Stage(CurrentStage);
        if (Payout->RoundRealtimeSeconds != -1.0f)
        {
            Payout->ServerWorldTimeStageEnd = Now() + Payout->RoundRealtimeSeconds;
        }
        UpdateEveryoneReady();
        break;
    }
    default:
        break;
    }
    if ((Type == EMultiplayerGameplayStage::RoundStart_SetupGameplay || Type == EMultiplayerGameplayStage::RoundComplete_FarShot) && ClockBpm > 120)
    {
        ShiftStagesFrom(CurrentStage, 1);
    }
    if (GameState.IsValid())
    {
        GameState->ForceNetUpdate();
    }
    const FMultiplayerGameStage* S = Stage(CurrentStage);
    UE_LOG(LogFDS, Display, TEXT("FDS: stage %s of %d, from %s; bars %d-%d, now bar %d (tick %d, %d BPM, sync tick %d); rounds %d, turn measures %d, DJ slots %d%s"),
        *StageLabel(CurrentStage), M->_gameStages.Num(), *Previous, S ? S->startMeasure : -1, S ? S->endMeasure : -1, ClockMeasure(), ClockTick(), ClockBpm,
        SyncTick, M->_numCoopFreestyleRounds, M->_coopTurnLengthMeasures, MaxNumDJs,
        CurrentDJ.IsValid() ? *FString::Printf(TEXT("; DJ player %d"), CurrentDJ->PlayerId) : TEXT(""));
}

// MPS slot 124 (FUN_1406241f0): Playing lasts the turn length, every other stage PerStageMeasureCount[type].
int32 UFDSLobbyAuthority::BarsFor(EMultiplayerGameplayStage Type) const
{
    if (Type == EMultiplayerGameplayStage::Playing)
    {
        return Mps() ? Mps()->_coopTurnLengthMeasures : -1;
    }
    if (bMainStage && Type == EMultiplayerGameplayStage::Results_XPPayout)
    {
        return MainStageXPResultsBars; // the Main Stage MPS's slot 124 (FUN_140624160): NumMeasuresForMainStageXPResults
    }
    const int32 TypeIndex = static_cast<int32>(Type);
    return TypeIndex < UE_ARRAY_COUNT(PerStageMeasureCount) ? PerStageMeasureCount[TypeIndex] : -1;
}

// FUN_140632360: the player holding a DJ index.
AHypePlayerState* UFDSLobbyAuthority::PlayerForDJIndex(int32 DJIndex) const
{
    if (DJIndex != -1)
    {
        for (AHypePlayerState* Player : Players())
        {
            if (Player->_djIndex == DJIndex)
            {
                return Player;
            }
        }
    }
    return nullptr;
}

// FUN_140632050's test: a turn counts while its DJ is present; a DJ-less stage counts when it has bars
// (FUN_140625a80).
bool UFDSLobbyAuthority::StageCounts(const FMultiplayerGameStage& S) const
{
    return S.djIndex != -1 ? PlayerForDJIndex(S.djIndex) != nullptr : BarsFor(S.Stage) != -1;
}

// FUN_14062ef40: from the start bar, each stage after the current one gets its bars back to back. Stages before the
// current one keep theirs; a turn whose DJ index has no player, and stages without a bar count, get none.
void UFDSLobbyAuthority::LayOutStages(int32 StartMeasure)
{
    UMultiplayerGameplayState* M = Mps();
    int32 Next = StartMeasure;
    for (int32 i = 0; i < M->_gameStages.Num(); ++i)
    {
        FMultiplayerGameStage& S = M->_gameStages[i];
        if (S.djIndex != -1 && !PlayerForDJIndex(S.djIndex))
        {
            continue;
        }
        if (i < CurrentStage)
        {
            Next = S.endMeasure;
            continue;
        }
        const int32 Bars = BarsFor(S.Stage);
        if (Bars != -1)
        {
            S.startMeasure = Next;
            Next += Bars;
            S.endMeasure = Next;
        }
    }
}

// FUN_140636bf0 tail: from the given stage on, every laid-out stage moves the given number of bars later (the stage
// itself only lengthens).
void UFDSLobbyAuthority::ShiftStagesFrom(int32 Index, int32 Bars)
{
    UMultiplayerGameplayState* M = Mps();
    for (int32 i = FMath::Max(Index, 0); i < M->_gameStages.Num(); ++i)
    {
        FMultiplayerGameStage& S = M->_gameStages[i];
        if (S.startMeasure != -1 && S.endMeasure != -1)
        {
            if (i != Index)
            {
                S.startMeasure += Bars;
            }
            S.endMeasure += Bars;
        }
    }
}

// A DJ left during a set (quit, crash, kick). Their stages no longer count (FUN_140632050), but their bars stay laid
// out: the server would sit in a performer-less turn until its last bar (up to 640 bars on the Main Stage), while the
// clients, which skip that turn themselves, wait in the next stage. FDS instead:
// - their current turn ends at the end of its current 32-bar block (from the turn's start);
// - their turns still to come are removed;
// and every later stage moves earlier by the bars taken out, so the next performer follows at once. The stage list is
// replicated, so the clients re-time from it.
void UFDSLobbyAuthority::DropPerformer(AHypePlayerState* Player)
{
    constexpr int32 Block = 32;
    if (!Player || Player->_team != EMultiplayerTeam::DJ || Player->_djIndex == -1 || CurrentStage <= 0 || !Mps())
    {
        return;
    }
    const int32 Index = Player->_djIndex;
    const int32 Measure = ClockMeasure();
    int32 Removed = 0;
    FString Done;
    for (int32 i = CurrentStage; Stage(i); ++i)
    {
        FMultiplayerGameStage& S = *Stage(i);
        if (S.djIndex != Index || S.startMeasure == -1 || S.endMeasure == -1)
        {
            continue;
        }
        if (i == CurrentStage)
        {
            if (S.Stage != EMultiplayerGameplayStage::Playing)
            {
                continue; // a 1-2 bar intro of their own: it runs out by itself
            }
            const int32 Played = FMath::Max(0, Measure - S.startMeasure);
            const int32 NewEnd = S.startMeasure + (Played / Block + 1) * Block;
            if (NewEnd < S.endMeasure)
            {
                const int32 Cut = S.endMeasure - NewEnd;
                ShiftStagesFrom(i, -Cut); // this stage ends sooner, every later one moves up
                Removed += Cut;
                Done += FString::Printf(TEXT(" turn %s now ends at bar %d (-%d);"), *StageLabel(i), NewEnd, Cut);
            }
            continue;
        }
        const int32 Bars = S.endMeasure - S.startMeasure;
        S.startMeasure = S.endMeasure = -1;
        ShiftStagesFrom(i, -Bars); // the stages after it move up
        Removed += Bars;
        Done += FString::Printf(TEXT(" %s removed (-%d);"), *StageLabel(i), Bars);
    }
    if (Removed > 0)
    {
        if (GameState.IsValid())
        {
            GameState->ForceNetUpdate();
        }
        UE_LOG(LogFDS, Display, TEXT("FDS: performer player %d (DJ index %d) left at bar %d:%s %d bar(s) taken out"), Player->PlayerId, Index, Measure,
            *Done, Removed);
    }
}

// FUN_140636bf0 case RoundComplete_FarShot. FUSER scores each player's best measure (FUN_140651a10: the mix
// weight from praise x1, snapshots x10, requests x5) and copies the winner's mix into _hottestMix (FUN_14061bf00).
// Called when ScoreHottestMix found no winner - nobody received reactions, snapshots or requests (any solo set).
// FUSER then moves Results up to the HottestMixIntro's first bar and drops both hottest-mix stages. But a client
// fades and ejects its decks only in HottestMixPlayback (FUN_140638680); the lobby entrance only dips their volume
// and nothing binds OnLobbyVolumeZero, so without that stage the DJ's stems play on into the lobby.
// CollapseHottestMixWithoutWinner (default True) does FUSER's collapse; False keeps the stages with an empty
// _hottestMix.
void UFDSLobbyAuthority::PlaceResultsWithoutHottestMix()
{
    if (!bCollapseHottestMixWithoutWinner)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: no hottest mix (nobody scored): hottest-mix stages kept so clients clear their decks"));
        return;
    }
    CollapseHottestMix(TEXT("nobody scored"));
}

// FUSER's collapse (FUN_140636bf0 case RoundComplete_FarShot): Results moves up to the HottestMixIntro's first bar and
// both hottest-mix stages are dropped. Clients keep their decks playing into the Lobby (no HottestMixPlayback eject).
void UFDSLobbyAuthority::CollapseHottestMix(const TCHAR* Why)
{
    UMultiplayerGameplayState* M = Mps();
    auto Find = [M](EMultiplayerGameplayStage Type) -> FMultiplayerGameStage*
    {
        return M->_gameStages.FindByPredicate([Type](const FMultiplayerGameStage& S) { return S.Stage == Type; });
    };
    FMultiplayerGameStage* Intro = Find(EMultiplayerGameplayStage::RoundComplete_HottestMixIntro);
    FMultiplayerGameStage* Playback = Find(EMultiplayerGameplayStage::RoundComplete_HottestMixPlayback);
    FMultiplayerGameStage* Results = Find(EMultiplayerGameplayStage::Results);
    if (!Intro || !Playback || !Results)
    {
        return;
    }
    Results->startMeasure = Intro->startMeasure;
    Results->endMeasure = Results->startMeasure + BarsFor(EMultiplayerGameplayStage::Results);
    Intro->startMeasure = Intro->endMeasure = -1;
    Playback->startMeasure = Playback->endMeasure = -1;
    FMultiplayerGameStage* Payout = Find(EMultiplayerGameplayStage::Results_XPPayout);
    if (Payout && Payout->startMeasure != -1)
    {
        Payout->startMeasure = Results->endMeasure; // the Main Stage's bar-timed payout follows Results
        Payout->endMeasure = Payout->startMeasure + BarsFor(EMultiplayerGameplayStage::Results_XPPayout);
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: no hottest mix (%s): Results moved to bars %d-%d"), Why, Results->startMeasure, Results->endMeasure);
}

// MPS slot +0x498 (FUN_140637660), entering SetupGameplay: the DJ order is shuffled, up to ten tries for an order
// different from the current one; then the queue follows the new indices (FUN_14062fed0).
void UFDSLobbyAuthority::ShuffleDJOrder()
{
    TArray<AHypePlayerState*> Queued;
    TArray<int32> Current;
    for (const TWeakObjectPtr<AHypePlayerState>& Player : DJQueue)
    {
        if (Player.IsValid())
        {
            Queued.Add(Player.Get());
            Current.Add(Player->_djIndex);
        }
    }
    if (Queued.Num() < 2)
    {
        return;
    }
    for (int32 Attempt = 0; Attempt < 10; ++Attempt)
    {
        TArray<int32> Order = Current;
        for (int32 i = Order.Num() - 1; i > 0; --i)
        {
            Order.Swap(i, FMath::RandRange(0, i));
        }
        if (Order != Current)
        {
            for (int32 i = 0; i < Queued.Num(); ++i)
            {
                Queued[i]->_djIndex = Order[i];
                Queued[i]->ForceNetUpdate();
            }
            LogPlayers(TEXT("DJ order shuffle"));
            return;
        }
    }
}

// FUN_140632050 (MPS slot +0x430), from the server's view: the stage a machine at this bar should be in.
// - The Lobby holds until its end time has passed.
// - Else the first stage whose bars contain this one (and that counts).
// - Else, looking on from the current stage, the next laid-out stage if it is still ahead and counts; else the first
//   stage not yet completed; else the Lobby.
int32 UFDSLobbyAuthority::StageForMeasure(int32 Measure) const
{
    const UMultiplayerGameplayState* M = Mps();
    const TArray<FMultiplayerGameStage>& Stages = M->_gameStages;
    if (Stages.Num() == 0)
    {
        return -1;
    }
    if (Stages[0].Stage == EMultiplayerGameplayStage::Lobby)
    {
        const float End = Stages[0].ServerWorldTimeStageEnd;
        if (End == -1.0f || Now() <= End)
        {
            return 0;
        }
    }
    for (int32 i = 0; i < Stages.Num(); ++i)
    {
        const FMultiplayerGameStage& S = Stages[i];
        if (S.startMeasure != -1 && S.endMeasure != -1 && S.startMeasure <= Measure && Measure < S.endMeasure && StageCounts(S))
        {
            return i;
        }
    }
    for (int32 i = FMath::Max(CurrentStage, 0); i < Stages.Num(); ++i)
    {
        const FMultiplayerGameStage& S = Stages[i];
        if (S.startMeasure != -1 && S.endMeasure != -1)
        {
            if (Measure < S.startMeasure && StageCounts(S))
            {
                return i;
            }
            break;
        }
    }
    for (int32 i = 0; i < Stages.Num(); ++i)
    {
        if (!Stages[i].ServerHasCompletedStage)
        {
            return i;
        }
    }
    return 0;
}

// The stage driver FUSER's dedicated build compiled out. Every tick the server takes the stage for the master bar,
// moving only forward or back to the Lobby (as each machine's FUN_140637fd0 does). Past the last laid-out bar the
// set is over: the server returns to the Lobby.
void UFDSLobbyAuthority::DriveStages()
{
    if (CurrentStage < 0 || !bClockRunning)
    {
        return;
    }
    const int32 Measure = ClockMeasure();
    int32 Next = StageForMeasure(Measure);
    if (CurrentStage > 0 && Next == CurrentStage)
    {
        const FMultiplayerGameStage* S = Stage(CurrentStage);
        bool bLaterStage = false;
        for (int32 i = CurrentStage + 1; Stage(i); ++i)
        {
            bLaterStage |= Stage(i)->startMeasure != -1 && StageCounts(*Stage(i));
        }
        if (S && S->endMeasure != -1 && Measure >= S->endMeasure && !bLaterStage)
        {
            const int32 Payout = Mps()->_gameStages.IndexOfByPredicate([](const FMultiplayerGameStage& Each)
            {
                return Each.Stage == EMultiplayerGameplayStage::Results_XPPayout;
            });
            if (bResultsXPPayout && S->Stage == EMultiplayerGameplayStage::Results && Payout > CurrentStage)
            {
                UE_LOG(LogFDS, Display, TEXT("FDS: results over at bar %d: XP payout"), Measure);
                Next = Payout;
            }
            else
            {
                UE_LOG(LogFDS, Display, TEXT("FDS: set complete at bar %d: back to the Lobby"), Measure);
                Next = 0;
            }
        }
        // Results_XPPayout has no bars: it ends at its end time, or once every queued DJ is ready in the results.
        // On the Main Stage a payout without queued DJs (every headliner left) runs its time: 0 of 0 is not "ready".
        const bool bPayoutReady = bEveryoneReady && !(bMainStage && DJQueue.Num() == 0);
        if (S && S->Stage == EMultiplayerGameplayStage::Results_XPPayout && ((S->ServerWorldTimeStageEnd != -1.0f && Now() > S->ServerWorldTimeStageEnd) || bPayoutReady))
        {
            UE_LOG(LogFDS, Display, TEXT("FDS: XP payout over (%s): back to the Lobby"), bEveryoneReady ? TEXT("everyone ready") : TEXT("time up"));
            Next = 0;
        }
    }
    if (Next >= 0 && Next != CurrentStage && (Next > CurrentStage || Next == 0))
    {
        SetStage(Next);
    }
}

// FUN_14062bac0 (ServerMarkLobbyComplete on the leader, or FDS's auto-start): in the Lobby, with DJs and everyone
// ready. The set starts at the first bar whose listener-delayed start is more than MinLobbyReadyTimer away (looking
// at most MaxLobbyReadyTimerMeasures bars ahead); the stages are laid out from there and the Lobby counts down to it.
bool UFDSLobbyAuthority::CompleteLobby(const TCHAR* Why)
{
    FMultiplayerGameStage* LobbyStage = Stage(CurrentStage);
    if (CurrentStageType() != EMultiplayerGameplayStage::Lobby || DJQueue.Num() == 0 || !bEveryoneReady || !bClockRunning)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: lobby completion (%s) refused: stage %s, %d DJs, everyone ready %s"), Why, *StageLabel(CurrentStage), DJQueue.Num(),
            bEveryoneReady ? TEXT("yes") : TEXT("no"));
        return false;
    }
    const int32 NowMeasure = ClockMeasure();
    int32 Bar = 0;
    float Wait = 0.0f;
    if (MaxLobbyReadyTimerMeasures > 1)
    {
        for (Bar = NowMeasure + 1;; ++Bar)
        {
            Wait = SecondsUntilTick((ClientNumMeasuresBehind + Bar) * TicksPerBar);
            if (Wait > MinLobbyReadyTimer || (Bar + 1) - NowMeasure >= MaxLobbyReadyTimerMeasures)
            {
                break;
            }
        }
    }
    // A public lobby timer that ends sooner than this countdown is kept (FUSER would re-time it a few bars later).
    if (bLobbyTimerRunning && LobbyStage->ServerWorldTimeStageEnd != -1.0f && LobbyStage->ServerWorldTimeStageEnd <= Now() + Wait)
    {
        bLobbyTimerRunning = false;
        UE_LOG(LogFDS, Display, TEXT("FDS: lobby complete (%s): the public lobby timer ends sooner (in %.2f s); kept"), Why,
            LobbyStage->ServerWorldTimeStageEnd - Now());
        return true;
    }
    SetStartMeasure = Bar + 1;
    LayOutStages(SetStartMeasure);
    LobbyStage->ServerWorldTimeStageEnd = Now() + Wait;
    LobbyStage->RoundRealtimeSeconds = MinLobbyReadyTimer;
    bLobbyTimerRunning = false;
    GameState->ForceNetUpdate();
    FString Layout;
    for (int32 i = 1; Stage(i); ++i)
    {
        if (Stage(i)->startMeasure != -1)
        {
            Layout += FString::Printf(TEXT(" %d:%d-%d"), i, Stage(i)->startMeasure, Stage(i)->endMeasure);
        }
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: lobby complete (%s) at bar %d: set starts at bar %d, lobby ends in %.2f s (world %.2f); layout%s"), Why, NowMeasure,
        SetStartMeasure, Wait, LobbyStage->ServerWorldTimeStageEnd, *Layout);
    return true;
}

// MPS slot 146 (FUN_140637a40): FUSER's timed public lobby start. It has no caller in the shipping build (the
// dedicated game flow was compiled out); FDS runs it every Lobby tick. Only in a public session (FUSER: the
// _gameServer's IsPublic, GameMode +0x330 vtable +0x320). PublicLobbyTimer=False turns it off.
// - With PublicLobbyTimerMinDJs (2) or more queued DJs and no Lobby end yet: the set starts at the bar that the time
//   PublicLobbyTimerSeconds (the Lobby's RoundRealtimeSeconds, 60) from now falls in, with no listener delay. The
//   Lobby ends at that bar's start and the stages are laid out from it.
// - With fewer DJs, a Lobby end and not everyone ready, the countdown is cancelled (FUN_1406203a0): the end goes back
//   to -1 and the seconds to 60. The laid-out bars stay, as in FUSER.
// Everyone ready still completes the Lobby (CompleteLobby), which cuts the countdown to MinLobbyReadyTimer (5 s). Once
// the timer runs out the set starts whether or not everyone is ready.
void UFDSLobbyAuthority::UpdatePublicLobbyTimer()
{
    if (!bPublicLobbyTimer || !IsPublicSession() || !bClockRunning || CurrentStageType() != EMultiplayerGameplayStage::Lobby)
    {
        return;
    }
    FMultiplayerGameStage* LobbyStage = Stage(CurrentStage);
    if (DJQueue.Num() < PublicLobbyTimerMinDJs)
    {
        if (LobbyStage->ServerWorldTimeStageEnd != -1.0f && !bEveryoneReady)
        {
            LobbyStage->ServerWorldTimeStageEnd = -1.0f;
            LobbyStage->RoundRealtimeSeconds = PublicLobbyTimerSeconds;
            bLobbyTimerRunning = false;
            GameState->ForceNetUpdate();
            UE_LOG(LogFDS, Display, TEXT("FDS: lobby countdown cancelled: %d DJ(s) queued and not everyone ready (public lobby timer)"), DJQueue.Num());
        }
        return;
    }
    if (LobbyStage->ServerWorldTimeStageEnd != -1.0f)
    {
        return;
    }
    const int32 NowTick = ClockTick();
    const int32 Bar = static_cast<int32>((NowTick + static_cast<double>(PublicLobbyTimerSeconds) * ClockBpm * (TicksPerBeat / 60.0)) / TicksPerBar);
    const float Wait = SecondsUntilTick(Bar * TicksPerBar);
    SetStartMeasure = Bar;
    LayOutStages(SetStartMeasure);
    LobbyStage->ServerWorldTimeStageEnd = Now() + Wait;
    LobbyStage->RoundRealtimeSeconds = PublicLobbyTimerSeconds;
    bLobbyTimerRunning = true;
    GameState->ForceNetUpdate();
    UE_LOG(LogFDS, Display, TEXT("FDS: public lobby timer: %d DJs queued at bar %d: set starts at bar %d, lobby ends in %.2f s (world %.2f)"), DJQueue.Num(),
        NowTick / TicksPerBar, SetStartMeasure, Wait, LobbyStage->ServerWorldTimeStageEnd);
}

// FUN_140705120: DJ while the DJ count is below MaxNumDJsInSession, else Spectator. FDS does not count the player
// itself, so a repeated ServerSetHypeUserId cannot demote a full DJ team (FUSER counted every DJ).
EMultiplayerTeam UFDSLobbyAuthority::TeamForNewPlayer(const AHypePlayerState* Player) const
{
    if (bMainStage)
    {
        return MainStageTeam(Player);
    }
    int32 DJs = 0;
    for (AHypePlayerState* Other : Players(Player))
    {
        DJs += Other->_team == EMultiplayerTeam::DJ ? 1 : 0;
    }
    return DJs < MaxNumDJs ? EMultiplayerTeam::DJ : EMultiplayerTeam::Spectator;
}

// FUN_1406963e0: a team change clears both ready bits.
void UFDSLobbyAuthority::SetTeam(AHypePlayerState* Player, EMultiplayerTeam Team)
{
    if (Player->_team == Team)
    {
        return;
    }
    Player->_team = Team;
    Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(ReadyBits(Player) & ~(ReadyLobbyBit | ReadyResultsBit));
    Player->ForceNetUpdate();
}

// FUN_140636630 (Lobby / Battle_WaitForPlayers): non-DJs get -1; DJs keep their order and are numbered from 0, with
// indices at or above the slot count set to -1. With a set start bar (a countdown running) the stages are laid out
// again from it, so a DJ who joins or leaves during the countdown gains or loses a turn.
void UFDSLobbyAuthority::AssignDJIndices()
{
    const EMultiplayerGameplayStage Type = CurrentStageType();
    if (Type != EMultiplayerGameplayStage::Lobby && Type != EMultiplayerGameplayStage::Battle_WaitForPlayers)
    {
        return;
    }
    TArray<AHypePlayerState*> DJs;
    for (AHypePlayerState* Player : Players())
    {
        if (Player->_team == EMultiplayerTeam::DJ)
        {
            DJs.Add(Player);
        }
        else if (Player->_djIndex != -1)
        {
            Player->_djIndex = -1;
            Player->ForceNetUpdate();
        }
    }
    if (bMainStage)
    {
        // MPS slot 147 (FUN_140637450): the DJ order is the headliner list's order.
        DJs.StableSort([this](const AHypePlayerState& A, const AHypePlayerState& B) { return HeadlinerRank(&A) < HeadlinerRank(&B); });
    }
    else
    {
        // existing indices first (stable), newcomers after them in join order
        DJs.StableSort([](const AHypePlayerState& A, const AHypePlayerState& B)
        {
            const int32 IA = A._djIndex < 0 ? MAX_int32 : A._djIndex;
            const int32 IB = B._djIndex < 0 ? MAX_int32 : B._djIndex;
            return IA < IB;
        });
    }
    for (int32 i = 0; i < DJs.Num(); ++i)
    {
        const int32 Index = i < MaxNumDJs ? i : -1;
        if (DJs[i]->_djIndex != Index)
        {
            DJs[i]->_djIndex = Index;
            DJs[i]->ForceNetUpdate();
        }
    }
    if (SetStartMeasure != -1)
    {
        LayOutStages(SetStartMeasure);
        if (GameState.IsValid())
        {
            GameState->ForceNetUpdate();
        }
    }
}

// FUN_140633500: DJs with an index form the queue (by index); everyone else spectates.
void UFDSLobbyAuthority::RebuildDJQueue()
{
    TArray<AHypePlayerState*> Queue;
    Spectators.Reset();
    for (AHypePlayerState* Player : Players())
    {
        if (Player->_team == EMultiplayerTeam::DJ && Player->_djIndex != -1)
        {
            Queue.Add(Player);
        }
        else
        {
            Spectators.Add(Player);
        }
    }
    Queue.Sort([](const AHypePlayerState& A, const AHypePlayerState& B) { return A._djIndex < B._djIndex; });
    DJQueue.Reset();
    for (AHypePlayerState* Player : Queue)
    {
        DJQueue.Add(Player);
    }
}

// FUN_1406383b0: in Lobby count queued DJs with the Lobby ready bit (Results_XPPayout: the Results bit); everyone is
// ready when all are, and in Lobby at least one is.
void UFDSLobbyAuthority::UpdateEveryoneReady()
{
    const EMultiplayerGameplayStage Type = CurrentStageType();
    const uint8 Bit = Type == EMultiplayerGameplayStage::Lobby ? ReadyLobbyBit : Type == EMultiplayerGameplayStage::Results_XPPayout ? ReadyResultsBit : 0;
    int32 Count = 0;
    for (const TWeakObjectPtr<AHypePlayerState>& Player : DJQueue)
    {
        Count += Bit && Player.IsValid() && (ReadyBits(Player.Get()) & Bit) ? 1 : 0;
    }
    const bool bReady = Bit && Count == DJQueue.Num() && (Type != EMultiplayerGameplayStage::Lobby || Count > 0);
    if (Count != ReadyCount || bReady != bEveryoneReady)
    {
        const bool bBecameReady = bReady && !bEveryoneReady;
        ReadyCount = Count;
        bEveryoneReady = bReady;
        UE_LOG(LogFDS, Display, TEXT("FDS: ready %d/%d DJs%s"), ReadyCount, DJQueue.Num(), bEveryoneReady ? TEXT(" - everyone ready") : TEXT(""));
        // The FUSER client never sends ServerMarkLobbyComplete by itself: once everyone is ready, FDS completes the
        // lobby for the leader unless AutoStart=False; a Lobby already counting down is left alone, except for the public lobby
        // timer, which everyone ready cuts short (FUN_14062bac0 does not check the end time).
        const FMultiplayerGameStage* LobbyStage = Stage(CurrentStage);
        if (bBecameReady && bAutoStart && Type == EMultiplayerGameplayStage::Lobby && LobbyStage &&
            (LobbyStage->ServerWorldTimeStageEnd == -1.0f || bLobbyTimerRunning))
        {
            CompleteLobby(TEXT("auto-start: everyone ready"));
        }
    }
}

// FUN_1406965a0: one leader; setting one clears the others.
void UFDSLobbyAuthority::SetSessionLeader(AHypePlayerState* Leader)
{
    for (AHypePlayerState* Player : Players())
    {
        const bool bLeader = Player == Leader;
        if (Player->_isSessionLeader != bLeader)
        {
            Player->_isSessionLeader = bLeader;
            Player->ForceNetUpdate();
        }
    }
}

void UFDSLobbyAuthority::OnPlayerJoined(AHypePlayerState* Player)
{
    // A DJPlayerState per player, created by the server as a dynamic replicated subobject of the PlayerState (as
    // FUSER's: DJPlayerState_<n>, outer the PlayerState, not a default subobject). ResultsState's template does not replicate;
    // FUSER replicated it at run time.
    if (!Player->_djPlayerState)
    {
        UDJPlayerState* DJState = NewObject<UDJPlayerState>(Player);
        DJState->SetIsReplicated(true);
        DJState->RegisterComponent();
        Player->_djPlayerState = DJState;
    }
    if (Player->_resultsState)
    {
        Player->_resultsState->SetIsReplicated(true);
    }
    Player->_serverAccessState = ServerAccessState;
    ResetResults(Player);
    if (!bClockRunning)
    {
        RestartClock(LobbyBPM); // a FUSER host's lobby master tick counts from the first arrival
    }
    SeedStateOfTheWorld(Player);
    // No team yet. In FUSER the team comes from the player's ServerSetHypeUserId (FUN_140696260 -> FUN_140705120),
    // seconds after the PlayerState exists everywhere. Set at admission, the team and DJ index arrived inside the
    // new PlayerState's first update and the clients' DJ queues (FUN_140633500, rebuilt on add and on the team /
    // index OnReps) left that player out (two DJs on the server, one in each client's queue).
    // A player that never sends its hype user id gets a team after TeamFallbackSeconds.
    PendingTeam.Add(Player, Now());
    bool bHasLeader = bMainStage; // the Main Stage has no session leader
    for (AHypePlayerState* Other : Players(Player))
    {
        bHasLeader |= Other->_isSessionLeader;
    }
    if (!bHasLeader)
    {
        SetSessionLeader(Player);
    }
    AssignDJIndices();
    RebuildDJQueue();
    UpdateEveryoneReady();
    LogPlayers(TEXT("joined"));
}

// The state-of-the-world snapshot (PlayerState._stateOfTheWorld, owner only). A client's local PlayerState applies
// it once (FUN_140699ac0) when AtMasterTick and InRound are set and the GlobalAudioState and DeckSoloMuteStates
// categories each hold an item; it then records its synced tick / round (PS +0x1040 / +0x1044) and reports
// _ServerSetHasSyncedStateOfTheWorld. Until then its action consumer ignores other players, and HottestMixPlayback
// does not clear its decks (FUN_140638680). FUSER's dedicated server kept these snapshots; FDS seeds the master state:
// the master BPM / key / mode, four decks neither muted nor soloed, stamped
// just after AtMasterTick.
// The late-join mix: a player joining while a mix plays gets the
// folded mix (FoldIntoMix) in the categories the sync applies (FUN_140626920): DeckPlayingState (SpawnDisc Play),
// DeckCuedState (SpawnDisc Queue), TrackGainState, AudioEffectState, ActiveGridInstruments, and the last
// GlobalAudioState / SoloMuteStates in place of the neutral ones. AtMasterTick is then the newest folded stamp (at most
// the master clock): the joiner's consumer (FUN_14069bc90) marks every item stamped at or before it completed without
// running it, so the actions already in the mix are not replayed and the later ones run. The copies keep their
// UniqueIds; FFastArraySerializerItem's copy resets the ReplicationID for the snapshot's arrays.
void UFDSLobbyAuthority::SeedStateOfTheWorld(AHypePlayerState* Player)
{
    FStateOfTheWorld& World = Player->_stateOfTheWorld;
    if (World.AtMasterTick != -1)
    {
        return;
    }
    const int32 Now = ClockTick();
    const bool bWithMix = Mix.Actions > 0 && Mix.Tick >= 0;
    int32 AtTick = Now > 0 ? Now - 1 : -2; // never -1, which means "no snapshot"
    if (bWithMix)
    {
        AtTick = Now >= 0 ? FMath::Min(Mix.Tick, Now) : Mix.Tick;
    }
    auto AddItem = [this, AtTick](FMultiplayerActionArray& Array, EMultiplayerActionType Type) -> FMultiplayerActionItemEntry&
    {
        FMultiplayerActionItemEntry& Item = Array.ActionItems.AddDefaulted_GetRef();
        Item.ActionType = Type;
        Item.UniqueId = NextActionId++;
        Item.MasterTimelineTick = AtTick + 1;
        return Item;
    };
    auto CopyInto = [](FMultiplayerActionArray& Array, const FMultiplayerActionItemEntry& From, TOptional<EActionAfterDiscSpawn> After = {})
    {
        FMultiplayerActionItemEntry& Item = Array.ActionItems.Add_GetRef(From);
        if (After.IsSet())
        {
            Item.actionAfterDiscSpawn = After.GetValue();
        }
        Array.MarkItemDirty(Item);
    };
    if (bWithMix && Mix.Global.IsSet())
    {
        CopyInto(World.GlobalAudioState, Mix.Global.GetValue());
    }
    else
    {
        FMultiplayerActionItemEntry& Global = AddItem(World.GlobalAudioState, EMultiplayerActionType::GlobalAudioState);
        Global.MasterBpm = ClockBpm;
        Global.MasterKey = MasterKey;
        Global.MasterMode = MasterMode;
        World.GlobalAudioState.MarkItemDirty(Global);
    }
    if (bWithMix && Mix.SoloMute.IsSet())
    {
        CopyInto(World.DeckSoloMuteStates, Mix.SoloMute.GetValue());
    }
    else
    {
        FMultiplayerActionItemEntry& SoloMute = AddItem(World.DeckSoloMuteStates, EMultiplayerActionType::SoloMuteStates);
        SoloMute.DeckStates.SetNum(4);
        World.DeckSoloMuteStates.MarkItemDirty(SoloMute);
    }
    if (bWithMix)
    {
        for (const TPair<uint8, FMultiplayerActionItemEntry>& Entry : Mix.Playing)
        {
            CopyInto(World.DeckPlayingState, Entry.Value, EActionAfterDiscSpawn::Play);
        }
        for (const TPair<uint8, FMultiplayerActionItemEntry>& Entry : Mix.Cued)
        {
            CopyInto(World.DeckCuedState, Entry.Value, EActionAfterDiscSpawn::Queue);
        }
        for (const TPair<uint8, FMultiplayerActionItemEntry>& Entry : Mix.Gain)
        {
            CopyInto(World.TrackGainState, Entry.Value);
        }
        for (const TPair<uint8, FMultiplayerActionItemEntry>& Entry : Mix.Effect)
        {
            CopyInto(World.AudioEffectState, Entry.Value);
        }
        for (const TPair<uint8, FMultiplayerActionItemEntry>& Entry : Mix.Instrument)
        {
            CopyInto(World.ActiveGridInstruments, Entry.Value);
        }
    }
    World.AtMasterTick = AtTick;
    World.InRound = RoundIndex;
    Player->ForceNetUpdate();
    const int32 Bpm = World.GlobalAudioState.ActionItems[0].MasterBpm;
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d state of the world seeded: tick %d, round %d, %d BPM, key %d, mode %d"), Player->PlayerId, AtTick,
        RoundIndex, Bpm, static_cast<int32>(World.GlobalAudioState.ActionItems[0].MasterKey), static_cast<int32>(World.GlobalAudioState.ActionItems[0].MasterMode));
    if (bWithMix)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d late-join mix of %d action(s) (master tick %d): %d playing, %d cued, %d gains, %d effects, %d instruments"),
            Player->PlayerId, Mix.Actions, Now, Mix.Playing.Num(), Mix.Cued.Num(), Mix.Gain.Num(), Mix.Effect.Num(), Mix.Instrument.Num());
    }
}

void UFDSLobbyAuthority::OnPlayerLeft(AHypePlayerState* Player)
{
    if (Player && Player->_isSessionLeader)
    {
        TArray<AHypePlayerState*> Remaining = Players(Player);
        if (Remaining.Num() > 0)
        {
            SetSessionLeader(Remaining[0]);
        }
        Player->_isSessionLeader = false;
    }
    Leaving = Player; // still in PlayerArray during Logout: excluded from the rebuild
    DropPerformer(Player);
    AssignDJIndices();
    RebuildDJQueue();
    UpdateEveryoneReady();
    LogPlayers(TEXT("left"));
    Leaving = nullptr;
}

void UFDSLobbyAuthority::OnSetHypeUserId(AHypePlayerState* Player)
{
    if (bMainStage)
    {
        // The id fuserd admitted stays (FDSMainStage); the RPC's value is the client's own claim.
        const int32 Pinned = FDSMainStage::PinnedUserId(Cast<APlayerController>(Player->GetOwner()));
        if (Pinned != INDEX_NONE && Player->_hypeUserId != Pinned)
        {
            UE_LOG(LogFDS, Warning, TEXT("FDS: main stage: player %d sent a hype user id other than its admitted one; kept the admitted id"), Player->PlayerId);
            Player->_hypeUserId = Pinned;
            Player->ForceNetUpdate();
        }
    }
    PendingTeam.Remove(Player);
    SetTeam(Player, TeamForNewPlayer(Player));
    AssignDJIndices();
    RebuildDJQueue();
    UpdateEveryoneReady();
    LogPlayers(TEXT("set hype user id"));
}

// ---------------------------------------------------------------------------------------------- DJ actions

// A DJ action RPC (HypeServerRpcs.cpp) as an item of the sender's _multiplayerActions (COND_SkipOwner: every other
// client). UniqueId comes from one session-wide counter, as recorded from a FUSER host (0, 1, 2, ...
// continuing across players); the fast array assigns the ReplicationID. ForceNetUpdate sends it at once - the
// listeners' one-bar buffer is the only margin.
void UFDSLobbyAuthority::OnRecordAction(AHypePlayerState* Player, FMultiplayerActionItemEntry& Item)
{
    Item.UniqueId = NextActionId++;
    // A late joiner's AtMasterTick is the mix's newest stamp (SeedStateOfTheWorld) and its consumer skips items at or
    // before it. A GlobalAudioState can come stamped earlier (the first of a set is stamped -1); it runs on arrival, so
    // restamping it after the mix delays nothing.
    if (Item.ActionType == EMultiplayerActionType::GlobalAudioState && Mix.Actions > 0 && Item.MasterTimelineTick <= Mix.Tick)
    {
        Item.MasterTimelineTick = Mix.Tick + 1;
    }
    FMultiplayerActionArray& Actions = Player->_multiplayerActions;
    FMultiplayerActionItemEntry& Added = Actions.ActionItems.Add_GetRef(Item);
    Actions.MarkItemDirty(Added);
    ActionRecordedAt.Add(Added.UniqueId, Now());
    Player->ForceNetUpdate();
    if (Added.ActionType == EMultiplayerActionType::RequestComplete)
    {
        CompleteRequest(Added, Player);
    }
    FoldIntoMix(Added);
    if (CurrentStage > 0)
    {
        Acted.Add(Player); // FUN_14069ea10's +0x1111
    }
    const FString Type = StaticEnum<EMultiplayerActionType>()->GetNameStringByValue(static_cast<int64>(Item.ActionType));
    int32& Count = ActionCounts.FindOrAdd(Type);
    ++Count;
    if (Count == 1 || Count % 50 == 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d action %s #%d (tick %d, master tick %d, %d items queued)"), Player->PlayerId, *Type, Count,
            Item.MasterTimelineTick, ClockTick(), Actions.ActionItems.Num());
    }
}

// An effect (FXParam) or instrument (InstrumentParam) parameter update: appended to the matching unreliable array of
// the sender, kept as a time series (listeners interpolate between the updates around their own clock).
void UFDSLobbyAuthority::OnRecordParam(AHypePlayerState* Player, const FMultiplayerUnreliableActionItemEntry& Item)
{
    const bool bInstrument = Item.ActionType == EMultiplayerUnreliableActionType::InstrumentParam;
    FMultiplayerUnreliableActionArray& Params = bInstrument ? Player->_multiplayerInstrumentParams : Player->_multiplayerFXParams;
    FMultiplayerUnreliableActionItemEntry& Added = Params.UnreliableActionItems.Add_GetRef(Item);
    Params.MarkItemDirty(Added);
    Player->ForceNetUpdate();
    if (CurrentStage > 0)
    {
        Acted.Add(Player);
    }
    const FString Type = bInstrument ? TEXT("InstrumentParam") : TEXT("FXParam");
    int32& Count = ActionCounts.FindOrAdd(Type);
    ++Count;
    if (Count == 1 || Count % 200 == 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d parameter %s #%d (slot %d, tick %d, master tick %d, %d updates queued)"), Player->PlayerId, *Type, Count,
            static_cast<int32>(bInstrument ? static_cast<uint8>(Item.InstSlot) : static_cast<uint8>(Item.FXSlot)), Item.MasterTimelineTick, ClockTick(),
            Params.UnreliableActionItems.Num());
    }
}

// FUN_140695870: registered by short name (FUN_1406922d0) and sent to every other player. FUSER loops over its promoted
// _hypePlayerStates; FDS keeps no promotion list on the server and sends to every other connected player.
void UFDSLobbyAuthority::OnAddCraftedInstrument(AHypePlayerState* Player, const FSerializableCraftedInstrumentCel& Data)
{
    CraftedRegistry.Add(Data.shortName, Data);
    int32 Sent = 0;
    for (AHypePlayerState* Other : Players(Player))
    {
        Other->ClientSendCraftedInstrumentCel(Data);
        ++Sent;
    }
    if (CurrentStage > 0)
    {
        Acted.Add(Player);
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d crafted instrument %s (asset %s, %d bytes): registered, sent to %d player(s)"), Player->PlayerId,
        *Data.shortName.ToString(), *Data.AssetShortName.ToString(), Data.RecordedDataBytes.Num(), Sent);
    ServeCraftedRequests();
}

// FUN_140695cb0: the request waits until the registry holds that instrument.
void UFDSLobbyAuthority::OnRequestCraftedInstrument(AHypePlayerState* Player, FName ShortName)
{
    PendingCrafted.FindOrAdd(Player).AddUnique(ShortName);
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d requests crafted instrument %s (%s)"), Player->PlayerId, *ShortName.ToString(),
        CraftedRegistry.Contains(ShortName) ? TEXT("registered") : TEXT("not registered yet: pending"));
    ServeCraftedRequests();
}

// FUN_14069f770: every pending name the registry holds goes to its requester through ClientSendCraftedInstrumentCel.
void UFDSLobbyAuthority::ServeCraftedRequests()
{
    for (auto It = PendingCrafted.CreateIterator(); It; ++It)
    {
        AHypePlayerState* Player = It.Key().Get();
        if (!Player)
        {
            It.RemoveCurrent();
            continue;
        }
        It.Value().RemoveAll([this, Player](const FName& Name)
        {
            const FSerializableCraftedInstrumentCel* Data = CraftedRegistry.Find(Name);
            if (Data)
            {
                Player->ClientSendCraftedInstrumentCel(*Data);
                UE_LOG(LogFDS, Display, TEXT("FDS: crafted instrument %s sent to player %d"), *Name.ToString(), Player->PlayerId);
            }
            return Data != nullptr;
        });
        if (It.Value().Num() == 0)
        {
            It.RemoveCurrent();
        }
    }
}

// ---------------------------------------------------------------------------------------------- results

// FUN_140651ba0(results, reset), at Lobby entry and for a joiner: fresh results (FUN_14063b970 sizes PraiseCounts for
// every praise type) and the per-measure tables sized [round][measure in the turn] from the set length
// (FUN_14064f330 / FUN_14064f290: _numCoopFreestyleRounds x _coopTurnLengthMeasures). The tables are what reactions,
// snapshots and requests count into (FUN_14065d0b0) and what the hottest mix is scored from.
void UFDSLobbyAuthority::ResetResults(AHypePlayerState* Player)
{
    UResultsState* Results = Player->_resultsState;
    const UMultiplayerGameplayState* M = Mps();
    if (!Results || !M)
    {
        return;
    }
    Results->_results = FGameplayResults();
    FGameplayResults& R = Results->_results;
    R.PraiseResults.PraiseCounts.SetNumZeroed(static_cast<int32>(EMultiplayerPraiseType::Num));
    const int32 Rounds = FMath::Max(M->_numCoopFreestyleRounds, 0);
    const int32 Measures = FMath::Max(M->_coopTurnLengthMeasures, 0);
    for (TArray<FPerMeasureResultSingleRoundStat>* Table : {&R.PerMeasureResults.RequestsCompleted, &R.PerMeasureResults.PraiseReceived,
                                                            &R.PerMeasureResults.SnapshotsReceived})
    {
        Table->SetNum(Rounds);
        for (FPerMeasureResultSingleRoundStat& Round : *Table)
        {
            Round.Stat.SetNumZeroed(Measures);
        }
    }
    PraiseGiven.Remove(Player);
    MostPraised.Remove(Player);
    Player->ForceNetUpdate();
}

// FUN_14065d0b0: count one at a master measure into the owner's table, if that measure lies in a laid-out Playing stage
// of the owner (stage DJ index = the owner's) with a round number: [round][measure - turn start].
bool UFDSLobbyAuthority::CountAtMeasure(AHypePlayerState* Owner, TArray<FPerMeasureResultSingleRoundStat>& Table, int32 Measure)
{
    const UMultiplayerGameplayState* M = Mps();
    const FMultiplayerGameStage* S = M ? M->_gameStages.FindByPredicate([Measure](const FMultiplayerGameStage& Stage)
    {
        return Stage.startMeasure != -1 && Stage.endMeasure != -1 && Stage.startMeasure <= Measure && Measure < Stage.endMeasure;
    }) : nullptr;
    if (!S || S->Stage != EMultiplayerGameplayStage::Playing || S->djIndex != Owner->_djIndex || S->RoundNumber < 0)
    {
        return false;
    }
    const int32 Offset = Measure - S->startMeasure;
    if (!Table.IsValidIndex(S->RoundNumber) || !Table[S->RoundNumber].Stat.IsValidIndex(Offset))
    {
        return false; // FUSER indexes without a check (an unsized table crashed a FUSER host)
    }
    ++Table[S->RoundNumber].Stat[Offset];
    return true;
}

// FUN_14065d190: the target's fans are the players whose most-praised DJ it is.
void UFDSLobbyAuthority::UpdateFans(AHypePlayerState* Target)
{
    if (!Target->_resultsState)
    {
        return;
    }
    FPraiseResults& Praise = Target->_resultsState->_results.PraiseResults;
    Praise.Fans.Reset();
    for (const auto& Entry : MostPraised)
    {
        if (Entry.Key.IsValid() && Entry.Value.Key.Get() == Target)
        {
            Praise.Fans.Add(Entry.Key.Get());
        }
    }
    Praise.TotalNumFans = Praise.Fans.Num();
}

// FUN_140695d60: in freestyle (MPS slot +0x3e8 false) the GameState multicasts the praises (FUN_140829310); the
// sender's PraiseSent counts once (FUN_140651740); each praise counts into its ForPlayer's results (FUN_1406515b0):
// PraiseCounts[type], TotalPraise, the sender's per-target tally and most-praised player, the per-measure table, fans.
void UFDSLobbyAuthority::OnSendPraises(AHypePlayerState* Sender, const TArray<FCollectedPraise>& Praises)
{
    if (bMainStage)
    {
        // FUSER's Main Stage clients batch their reactions (ClientRhythmojiCollectionTime) and simulate the crowd
        // from MainStageRhythmojiRates (OnRep -> FUN_140695330) rather than from every reaction: the server tallies
        // them instead of multicasting each one to everyone (MainStageMulticastPraises=True still multicasts).
        RhythmojiCounts.SetNumZeroed(static_cast<int32>(EMultiplayerPraiseType::Num));
        for (const FCollectedPraise& Praise : Praises)
        {
            const int32 Type = static_cast<int32>(Praise.praiseType);
            if (RhythmojiCounts.IsValidIndex(Type))
            {
                ++RhythmojiCounts[Type];
            }
        }
    }
    if (GameState.IsValid() && (!bMainStage || bMainStageMulticastPraises))
    {
        GameState->NetMulticastPraises(Praises);
    }
    if (Sender->_resultsState)
    {
        ++Sender->_resultsState->_results.PraiseResults.PraiseSent;
    }
    int32 Counted = 0;
    for (const FCollectedPraise& Praise : Praises)
    {
        AHypePlayerState* Target = Praise.ForPlayer;
        if (!Target || Target->IsPendingKill() || !Target->_resultsState)
        {
            continue;
        }
        FGameplayResults& R = Target->_resultsState->_results;
        const int32 Type = static_cast<int32>(Praise.praiseType);
        if (R.PraiseResults.PraiseCounts.IsValidIndex(Type))
        {
            ++R.PraiseResults.PraiseCounts[Type];
        }
        ++R.PraiseResults.TotalPraise;
        int32& Given = PraiseGiven.FindOrAdd(Sender).FindOrAdd(Target);
        ++Given;
        TPair<TWeakObjectPtr<AHypePlayerState>, int32>& Most = MostPraised.FindOrAdd(Sender);
        if (!Most.Key.IsValid() || Given > Most.Value)
        {
            Most = TPair<TWeakObjectPtr<AHypePlayerState>, int32>(Target, Given);
        }
        Counted += CountAtMeasure(Target, R.PerMeasureResults.PraiseReceived, Praise.Measure) ? 1 : 0;
        UpdateFans(Target);
        Target->ForceNetUpdate();
    }
    int32& Count = ActionCounts.FindOrAdd(TEXT("Praise"));
    Count += Praises.Num();
    if (Count == Praises.Num() || Count % 50 < Praises.Num())
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d sent %d praise(s) (%d in a turn's table; %d this set)"), Sender->PlayerId, Praises.Num(), Counted, Count);
    }
}

// FUN_140696080 / FUN_140651760: TotalNumSnapshotsTaken and the per-measure snapshot table of the player snapped.
void UFDSLobbyAuthority::OnSendSnapshotTaken(AHypePlayerState* Sender, AHypePlayerState* ToPlayer, int32 AtMeasure)
{
    if (!ToPlayer || ToPlayer->IsPendingKill() || !ToPlayer->_resultsState)
    {
        return;
    }
    FGameplayResults& R = ToPlayer->_resultsState->_results;
    ++R.SnapshotResults.TotalNumSnapshotsTaken;
    const bool bCounted = CountAtMeasure(ToPlayer, R.PerMeasureResults.SnapshotsReceived, AtMeasure);
    ToPlayer->ForceNetUpdate();
    ++ActionCounts.FindOrAdd(TEXT("Snapshot"));
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d took a snapshot of player %d at measure %d%s"), Sender->PlayerId, ToPlayer->PlayerId, AtMeasure,
        bCounted ? TEXT("") : TEXT(" (outside that player's turn: not in the table)"));
}

// ---------------------------------------------------------------------------------------------- mix and hottest mix

// Each recorded action as it changes the desk, by what the consumer's applier does with it (FUN_1406265e0):
// SpawnDisc Play / Queue (FUN_1405f4c00) sets the track's playing / cued disc; EjectDisc
// (FUN_1405f4520) clears the playing one; HotSwapOnDeck (FUN_1405f4a30) swaps the two; DoDropQueue (FUN_1405f3da0)
// plays every cued disc; EjectQueue (FUN_1405f4460) clears the cued one; the rest keep the latest per track / slot.
void UFDSLobbyAuthority::FoldIntoMix(const FMultiplayerActionItemEntry& Item)
{
    const uint8 Track = static_cast<uint8>(Item.trackType);
    switch (Item.ActionType)
    {
    case EMultiplayerActionType::SpawnDisc:
        if (Item.actionAfterDiscSpawn == EActionAfterDiscSpawn::Play)
        {
            Mix.Playing.Add(Track, Item);
        }
        else if (Item.actionAfterDiscSpawn == EActionAfterDiscSpawn::Queue)
        {
            Mix.Cued.Add(Track, Item);
        }
        break;
    case EMultiplayerActionType::EjectDisc:
        Mix.Playing.Remove(Track);
        break;
    case EMultiplayerActionType::HotSwapOnDeck:
    {
        TOptional<FMultiplayerActionItemEntry> WasPlaying;
        if (FMultiplayerActionItemEntry* P = Mix.Playing.Find(Track))
        {
            WasPlaying = *P;
        }
        Mix.Playing.Remove(Track);
        if (FMultiplayerActionItemEntry* C = Mix.Cued.Find(Track))
        {
            FMultiplayerActionItemEntry Now = *C;
            Now.actionAfterDiscSpawn = EActionAfterDiscSpawn::Play;
            Mix.Playing.Add(Track, Now);
            Mix.Cued.Remove(Track);
        }
        if (WasPlaying.IsSet())
        {
            WasPlaying->actionAfterDiscSpawn = EActionAfterDiscSpawn::Queue;
            Mix.Cued.Add(Track, WasPlaying.GetValue());
        }
        break;
    }
    case EMultiplayerActionType::DoDropQueue:
        for (TPair<uint8, FMultiplayerActionItemEntry>& Cued : Mix.Cued)
        {
            Cued.Value.actionAfterDiscSpawn = EActionAfterDiscSpawn::Play;
            Mix.Playing.Add(Cued.Key, Cued.Value);
        }
        Mix.Cued.Reset();
        break;
    case EMultiplayerActionType::EjectQueue:
        Mix.Cued.Remove(Track);
        break;
    case EMultiplayerActionType::EffectSlotState:
        Mix.Effect.Add(static_cast<uint8>(Item.slot), Item);
        break;
    case EMultiplayerActionType::GlobalAudioState:
        Mix.Global = Item;
        break;
    case EMultiplayerActionType::SoloMuteStates:
        Mix.SoloMute = Item;
        break;
    case EMultiplayerActionType::TrackGain:
        Mix.Gain.Add(Track, Item);
        break;
    case EMultiplayerActionType::CurrentCraftedInstrument:
        if (Item.InstrumentCelData)
        {
            Mix.Instrument.Add(static_cast<uint8>(Item.InstSlot), Item);
        }
        else
        {
            Mix.Instrument.Remove(static_cast<uint8>(Item.InstSlot));
        }
        break;
    default:
        return; // RequestComplete / TrickToDeck do not change the mix
    }
    Mix.Tick = FMath::Max(Mix.Tick, Item.MasterTimelineTick);
    ++Mix.Actions;
    bMixDirty = true;
}

void UFDSLobbyAuthority::ResetMix(const TCHAR* Why)
{
    if (Mix.Actions > 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: mix cleared (%s) after %d action(s)"), Why, Mix.Actions);
    }
    Mix = FFDSMix();
    bMixDirty = false;
}

// One FMultiplayerMeasureHistoryEntry as HottestMixPlayback replays it (FUN_140638680): CelShortNames[0..3] play on
// Deck0-3 (positional; empty when no deck plays), the master BPM / key / mode, the DJ, and AudioEffectState[i] on effect
// slot i + 3 (Deck0Slot..MasterSlot; an entry without effectData clears the slot).
FMultiplayerMeasureHistoryEntry UFDSLobbyAuthority::MakeHistoryEntry() const
{
    FMultiplayerMeasureHistoryEntry Entry;
    if (Mix.Playing.Num() > 0)
    {
        for (uint8 Deck = static_cast<uint8>(ETrackType::Deck0); Deck <= static_cast<uint8>(ETrackType::Deck3); ++Deck)
        {
            const FMultiplayerActionItemEntry* Playing = Mix.Playing.Find(Deck);
            Entry.CelShortNames.Add(Playing ? Playing->celShortName : NAME_None);
        }
    }
    Entry.BPM = Mix.Global.IsSet() ? Mix.Global->MasterBpm : ClockBpm;
    Entry.Key = Mix.Global.IsSet() ? Mix.Global->MasterKey : MasterKey;
    Entry.KeyMode = Mix.Global.IsSet() ? Mix.Global->MasterMode : MasterMode;
    Entry.DJForMeasure = CurrentDJ.Get();
    for (uint8 Slot = static_cast<uint8>(EAudioEffectSlot::Deck0Slot); Slot <= static_cast<uint8>(EAudioEffectSlot::MasterSlot); ++Slot)
    {
        const FMultiplayerActionItemEntry* Effect = Mix.Effect.Find(Slot);
        Entry.AudioEffectState.Add(Effect ? Effect->SlotInfo : FAudioEffectInfo());
    }
    return Entry;
}

// During a Playing stage, the history entry of the current master measure follows the mix (on every change and every
// new measure).
void UFDSLobbyAuthority::RecordMixHistory()
{
    if (CurrentStageType() != EMultiplayerGameplayStage::Playing)
    {
        return;
    }
    const int32 Measure = ClockMeasure();
    if (Measure < 0 || (!bMixDirty && Measure == LastHistoryMeasure))
    {
        return;
    }
    MixHistory.Add(Measure, MakeHistoryEntry());
    LastHistoryMeasure = Measure;
    bMixDirty = false;
}

// FUN_140636bf0 case RoundComplete_FarShot: each player is scored (FUN_140651a10 -> FUN_140657910): every measure of
// their turns weighs praise x HottestMixWeightPerPraise (1) + snapshots x ...PerSnapshot (10) + requests x
// ...PerRequest (5), or ...PerComboRequest (10) from two requests. The best measure's history entry (FUN_14062b950, only
// with CelShortNames) becomes MyHottestMix with that weight and the measure's praise / snapshot / request counts; the
// highest weight wins (FUN_140651cd0: IsRoundHottestMix) and is copied to MPS._hottestMix (FUN_14061bf00).
bool UFDSLobbyAuthority::ScoreHottestMix()
{
    constexpr int32 WeightPerPraise = 1, WeightPerSnapshot = 10, WeightPerRequest = 5, WeightPerComboRequest = 10;
    UMultiplayerGameplayState* M = Mps();
    AHypePlayerState* Winner = nullptr;
    for (AHypePlayerState* Player : Players())
    {
        UResultsState* Results = Player->_resultsState;
        if (!Results)
        {
            continue;
        }
        FGameplayResults& R = Results->_results;
        const FPerMeasureResults& Tables = R.PerMeasureResults;
        int32 BestRound = -1, BestOffset = -1, BestWeight = -1;
        for (int32 Round = 0; Round < Tables.PraiseReceived.Num(); ++Round)
        {
            for (int32 Offset = 0; Offset < Tables.PraiseReceived[Round].Stat.Num(); ++Offset)
            {
                auto At = [Round, Offset](const TArray<FPerMeasureResultSingleRoundStat>& Table)
                {
                    return Table.IsValidIndex(Round) && Table[Round].Stat.IsValidIndex(Offset) ? Table[Round].Stat[Offset] : 0;
                };
                const int32 Requests = At(Tables.RequestsCompleted);
                const int32 Weight = At(Tables.PraiseReceived) * WeightPerPraise + At(Tables.SnapshotsReceived) * WeightPerSnapshot
                    + Requests * (Requests < 2 ? WeightPerRequest : WeightPerComboRequest);
                if (BestWeight == -1 || Weight > BestWeight)
                {
                    BestRound = Round;
                    BestOffset = Offset;
                    BestWeight = Weight;
                }
            }
        }
        if (BestWeight <= 0)
        {
            continue;
        }
        const FMultiplayerGameStage* Turn = M->_gameStages.FindByPredicate([Player, BestRound](const FMultiplayerGameStage& S)
        {
            return S.Stage == EMultiplayerGameplayStage::Playing && S.djIndex == Player->_djIndex && S.RoundNumber == BestRound;
        });
        const FMultiplayerMeasureHistoryEntry* Entry = Turn && Turn->startMeasure != -1 ? MixHistory.Find(Turn->startMeasure + BestOffset) : nullptr;
        if (!Entry || Entry->CelShortNames.Num() == 0)
        {
            UE_LOG(LogFDS, Display, TEXT("FDS: player %d best measure weighs %d but has no mix history"), Player->PlayerId, BestWeight);
            continue;
        }
        R.MyHottestMix = *Entry;
        R.MyHottestMix.HottestMixWeight = BestWeight;
        R.MyHottestMix.HottestMixPraise = Tables.PraiseReceived[BestRound].Stat[BestOffset];
        R.MyHottestMix.HottestMixSnapshots = Tables.SnapshotsReceived.IsValidIndex(BestRound) && Tables.SnapshotsReceived[BestRound].Stat.IsValidIndex(BestOffset)
            ? Tables.SnapshotsReceived[BestRound].Stat[BestOffset] : 0;
        R.MyHottestMix.HottestMixRequests = Tables.RequestsCompleted.IsValidIndex(BestRound) && Tables.RequestsCompleted[BestRound].Stat.IsValidIndex(BestOffset)
            ? Tables.RequestsCompleted[BestRound].Stat[BestOffset] : 0;
        Results->_mainStageResults.MyHottestMix = R.MyHottestMix;
        Player->ForceNetUpdate();
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d hottest mix: round %d measure %d (+%d), weight %d (praise %d, snapshots %d, requests %d), %d cels"),
            Player->PlayerId, BestRound, Turn->startMeasure + BestOffset, BestOffset, BestWeight, R.MyHottestMix.HottestMixPraise,
            R.MyHottestMix.HottestMixSnapshots, R.MyHottestMix.HottestMixRequests, Entry->CelShortNames.Num());
        if (!Winner || BestWeight > Winner->_resultsState->_results.MyHottestMix.HottestMixWeight)
        {
            Winner = Player;
        }
    }
    if (!Winner)
    {
        return false;
    }
    FGameplayResults& W = Winner->_resultsState->_results;
    W.MyHottestMix.IsRoundHottestMix = true;
    Winner->_resultsState->_mainStageResults.MyHottestMix.IsRoundHottestMix = true;
    M->_hottestMix = W.MyHottestMix;
    Winner->ForceNetUpdate();
    UE_LOG(LogFDS, Display, TEXT("FDS: the round's hottest mix is player %d's (weight %d)"), Winner->PlayerId, W.MyHottestMix.HottestMixWeight);
    return true;
}

// ---------------------------------------------------------------------------------------------- live requests

namespace
{
    // FUSER DefaultGame.ini [/Script/Hype.HypeMultiplayerSettings] live-request timing (bars).
    constexpr int32 MeasuresToWaitForNewRequests = 1;
    constexpr int32 LiveRequestMinDurationMeasures = 2;
    constexpr int32 MeasuresBeforeEndOfTurnToFailLiveRequests = 1;
}

TArray<FMultiplayerLiveRequest>* UFDSLobbyAuthority::LiveRequests() const
{
    return RequestMgrObject.IsValid() ? &RequestMgrObject->_multiplayerLiveRequests : nullptr;
}

// FUN_140696820 / FUN_140651ce0: a request to the current DJ merges into an entry of the same type and content
// (case-insensitive; RequestedBy += sender) or is appended with ForPlayer = the DJ, RequestedBy = [sender], the next
// UniqueId and startMeasure / AvailabilityDuration unset (-1); the compiled-out scheduler then gave it its measures.
// The DJ's RequestsResults count it on arrival (FDS's reading of "requests received"; FUSER's writer was compiled out).
void UFDSLobbyAuthority::OnSubmitRequest(AHypePlayerState* Sender, const FMultiplayerLiveRequest& Request, AHypePlayerState* ToDJ)
{
    TArray<FMultiplayerLiveRequest>* List = LiveRequests();
    if (!List || !ToDJ || ToDJ != CurrentDJ.Get())
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d request '%s' dropped: not for the current DJ"), Sender->PlayerId, *Request.ContentName);
        return;
    }
    FMultiplayerLiveRequest* Same = List->FindByPredicate([&Request](const FMultiplayerLiveRequest& Entry)
    {
        return Entry.Type == Request.Type && Entry.ContentName.Equals(Request.ContentName, ESearchCase::IgnoreCase);
    });
    if (Same)
    {
        Same->RequestedBy.Add(Sender);
    }
    else
    {
        FMultiplayerLiveRequest& Added = List->Add_GetRef(Request);
        Added.ForPlayer = ToDJ;
        Added.RequestedBy = {Sender};
        Added.UniqueId = NextRequestId++;
        Added.startMeasure = -1;
        Added.AvailabilityDuration = -1;
    }
    if (UResultsState* Results = ToDJ->_resultsState)
    {
        FRequestsResults& R = Results->_results.RequestsResults;
        R.RequestsCounts.SetNumZeroed(FMath::Max(R.RequestsCounts.Num(), static_cast<int32>(EMultiplayerLiveRequestType::Num)));
        const int32 Type = static_cast<int32>(Request.Type);
        if (R.RequestsCounts.IsValidIndex(Type))
        {
            ++R.RequestsCounts[Type];
        }
        ++R.TotalNumRequestsReceived;
        ToDJ->ForceNetUpdate();
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d requests %s '%s' from DJ player %d (%s; %d in the list)"), Sender->PlayerId,
        *StaticEnum<EMultiplayerLiveRequestType>()->GetNameStringByValue(static_cast<int64>(Request.Type)), *Request.ContentName, ToDJ->PlayerId,
        Same ? TEXT("merged") : TEXT("new"), List->Num());
    ScheduleRequests();
}

// FUSER's compiled-out request scheduler, rebuilt: during a Playing stage each
// unscheduled request starts MeasuresToWaitForNewRequests after the current bar and lasts until
// MeasuresBeforeEndOfTurnToFailLiveRequests before the turn's end, at least LiveRequestMinDurationMeasures.
void UFDSLobbyAuthority::ScheduleRequests()
{
    TArray<FMultiplayerLiveRequest>* List = LiveRequests();
    const FMultiplayerGameStage* S = Stage(CurrentStage);
    if (!List || !S || S->Stage != EMultiplayerGameplayStage::Playing)
    {
        return;
    }
    bool bChanged = false;
    for (FMultiplayerLiveRequest& Entry : *List)
    {
        if (Entry.startMeasure != -1 && Entry.AvailabilityDuration != -1)
        {
            continue;
        }
        Entry.startMeasure = ClockMeasure() + MeasuresToWaitForNewRequests;
        Entry.AvailabilityDuration = FMath::Max(LiveRequestMinDurationMeasures, S->endMeasure - MeasuresBeforeEndOfTurnToFailLiveRequests - Entry.startMeasure);
        bChanged = true;
        UE_LOG(LogFDS, Display, TEXT("FDS: request #%d '%s' scheduled: bars %d +%d (turn ends at %d)"), Entry.UniqueId, *Entry.ContentName, Entry.startMeasure,
            Entry.AvailabilityDuration, S->endMeasure);
    }
    if (bChanged)
    {
        GameState->ForceNetUpdate();
    }
}

// The DJ's ServerUpdateRequestCompletion (recorded as action 11 for listeners): the request leaves the list; a passed
// one counts into the DJ's RequestsCompleted table at its measure (what the hottest mix weighs).
void UFDSLobbyAuthority::CompleteRequest(const FMultiplayerActionItemEntry& Item, AHypePlayerState* DJ)
{
    TArray<FMultiplayerLiveRequest>* List = LiveRequests();
    const int32 Index = List ? List->IndexOfByPredicate([&Item](const FMultiplayerLiveRequest& Entry) { return Entry.UniqueId == Item.uniqueRequestId; }) : INDEX_NONE;
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: completion of request #%d: not in the list"), Item.uniqueRequestId);
        return;
    }
    const bool bPassed = Item.completionType == EMultiplayerLiveRequestCompletionType::Passed;
    const bool bCounted = bPassed && DJ->_resultsState
        && CountAtMeasure(DJ, DJ->_resultsState->_results.PerMeasureResults.RequestsCompleted, Item.MasterTimelineTick / TicksPerBar);
    UE_LOG(LogFDS, Display, TEXT("FDS: request #%d '%s' %s by DJ player %d%s"), Item.uniqueRequestId, *(*List)[Index].ContentName,
        bPassed ? TEXT("passed") : TEXT("failed"), DJ->PlayerId, bCounted ? TEXT(" (counted)") : TEXT(""));
    List->RemoveAt(Index);
    GameState->ForceNetUpdate();
    DJ->ForceNetUpdate();
}

// The end of a turn (and the Lobby): every remaining request leaves the list.
void UFDSLobbyAuthority::ClearRequests(const TCHAR* Why)
{
    TArray<FMultiplayerLiveRequest>* List = LiveRequests();
    if (List && List->Num() > 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: %d request(s) cleared (%s)"), List->Num(), Why);
        List->Reset();
        GameState->ForceNetUpdate();
    }
}

// Parameter updates older than the listener window are dropped, except the newest one per slot: a listener with no
// update before its clock resets that effect's parameters to 0 (FUN_14061d740).
static int32 PruneParams(FMultiplayerUnreliableActionArray& Params, int32 OldestKept)
{
    TMap<int32, int32> NewestOld; // slot -> newest tick older than OldestKept
    for (const FMultiplayerUnreliableActionItemEntry& Item : Params.UnreliableActionItems)
    {
        const int32 Slot = Item.ActionType == EMultiplayerUnreliableActionType::InstrumentParam ? static_cast<int32>(Item.InstSlot) : static_cast<int32>(Item.FXSlot);
        if (Item.MasterTimelineTick < OldestKept)
        {
            int32& Newest = NewestOld.FindOrAdd(Slot, MIN_int32);
            Newest = FMath::Max(Newest, Item.MasterTimelineTick);
        }
    }
    const int32 Before = Params.UnreliableActionItems.Num();
    Params.UnreliableActionItems.RemoveAll([&](const FMultiplayerUnreliableActionItemEntry& Item)
    {
        const int32 Slot = Item.ActionType == EMultiplayerUnreliableActionType::InstrumentParam ? static_cast<int32>(Item.InstSlot) : static_cast<int32>(Item.FXSlot);
        return Item.MasterTimelineTick < OldestKept && Item.MasterTimelineTick != NewestOld.FindRef(Slot);
    });
    const int32 Removed = Before - Params.UnreliableActionItems.Num();
    if (Removed > 0)
    {
        Params.MarkArrayDirty();
    }
    return Removed;
}

// Every 10 s: an item recorded over 30 s ago that the master clock
// passed ClientNumMeasuresBehind + 3 bars ago has been consumed by every connected listener; one stamped over two bars
// ahead of the clock is left from before a timeline restart. Both are removed (the clients' fast arrays see deletes).
void UFDSLobbyAuthority::PruneActions()
{
    if (Now() < NextPruneAt)
    {
        return;
    }
    NextPruneAt = Now() + 10.0f;
    const int32 Master = ClockTick();
    if (Master < 0)
    {
        return;
    }
    const int32 Keep = (ClientNumMeasuresBehind + 3) * TicksPerBar;
    int32 Removed = 0;
    int32 Left = 0;
    for (AHypePlayerState* Player : Players())
    {
        FMultiplayerActionArray& Actions = Player->_multiplayerActions;
        const int32 Before = Actions.ActionItems.Num();
        Actions.ActionItems.RemoveAll([&](const FMultiplayerActionItemEntry& Item)
        {
            const float* RecordedAt = ActionRecordedAt.Find(Item.UniqueId);
            const bool bConsumed = RecordedAt && Now() - *RecordedAt > 30.0f && Master > Item.MasterTimelineTick + Keep;
            const bool bStale = Item.MasterTimelineTick > Master + 2 * TicksPerBar;
            if (bConsumed || bStale)
            {
                ActionRecordedAt.Remove(Item.UniqueId);
                return true;
            }
            return false;
        });
        if (Actions.ActionItems.Num() != Before)
        {
            Removed += Before - Actions.ActionItems.Num();
            Actions.MarkArrayDirty();
        }
        Left += Actions.ActionItems.Num();
        Removed += PruneParams(Player->_multiplayerFXParams, Master - Keep);
        Removed += PruneParams(Player->_multiplayerInstrumentParams, Master - Keep);
    }
    if (Removed > 0)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: pruned %d consumed action item(s) at master tick %d; %d left"), Removed, Master, Left);
    }
}

// Back in the Lobby the timeline restarts: the set's items are dropped and the per-type counts reported.
void UFDSLobbyAuthority::ClearActions(const TCHAR* Why)
{
    int32 Removed = 0;
    for (AHypePlayerState* Player : Players())
    {
        Removed += Player->_multiplayerActions.ActionItems.Num();
        if (Player->_multiplayerActions.ActionItems.Num() > 0)
        {
            Player->_multiplayerActions.ActionItems.Reset();
            Player->_multiplayerActions.MarkArrayDirty();
        }
        for (FMultiplayerUnreliableActionArray* Params : {&Player->_multiplayerFXParams, &Player->_multiplayerInstrumentParams})
        {
            Removed += Params->UnreliableActionItems.Num();
            if (Params->UnreliableActionItems.Num() > 0)
            {
                Params->UnreliableActionItems.Reset();
                Params->MarkArrayDirty();
            }
        }
    }
    ActionRecordedAt.Reset();
    FString Counts;
    for (const TPair<FString, int32>& Entry : ActionCounts)
    {
        Counts += FString::Printf(TEXT("%s%s %d"), Counts.IsEmpty() ? TEXT("") : TEXT(", "), *Entry.Key, Entry.Value);
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: actions recorded this set: %s; %d item(s) cleared (%s)"), Counts.IsEmpty() ? TEXT("none") : *Counts, Removed, Why);
    ActionCounts.Reset();
}

// The second half of the lobby DJ refresh after a set (see SetStage, Lobby): the DJs return to the DJ team in their
// previous order, as long as they are still Spectators in the Lobby and slots are free.
void UFDSLobbyAuthority::RestoreDJsAfterSet()
{
    if (DJsToRestore.Num() == 0 || Now() < DJRestoreAt)
    {
        return;
    }
    TArray<TWeakObjectPtr<AHypePlayerState>> Restore = MoveTemp(DJsToRestore);
    DJsToRestore.Reset();
    if (CurrentStageType() != EMultiplayerGameplayStage::Lobby)
    {
        return;
    }
    int32 Restored = 0;
    for (const TWeakObjectPtr<AHypePlayerState>& Player : Restore)
    {
        if (Player.IsValid() && Player->_team == EMultiplayerTeam::Spectator && TeamForNewPlayer(Player.Get()) == EMultiplayerTeam::DJ)
        {
            SetTeam(Player.Get(), EMultiplayerTeam::DJ);
            AssignDJIndices();
            ++Restored;
        }
    }
    RebuildDJQueue();
    UpdateEveryoneReady();
    LogPlayers(*FString::Printf(TEXT("lobby DJ refresh (%d restored)"), Restored));
}

// Players admitted TeamFallbackSeconds ago that never sent ServerSetHypeUserId get their team now.
void UFDSLobbyAuthority::AssignPendingTeams()
{
    bool bChanged = false;
    for (auto It = PendingTeam.CreateIterator(); It; ++It)
    {
        AHypePlayerState* Player = It.Key().Get();
        if (!Player || Player->IsPendingKill())
        {
            It.RemoveCurrent();
            continue;
        }
        if (Now() - It.Value() >= TeamFallbackSeconds)
        {
            It.RemoveCurrent();
            if (Player->_team == EMultiplayerTeam::Unknown)
            {
                UE_LOG(LogFDS, Display, TEXT("FDS: player %d sent no hype user id within %.0f s: team assigned by the server"), Player->PlayerId, TeamFallbackSeconds);
                SetTeam(Player, TeamForNewPlayer(Player));
                bChanged = true;
            }
        }
    }
    if (bChanged)
    {
        AssignDJIndices();
        RebuildDJQueue();
        UpdateEveryoneReady();
        LogPlayers(TEXT("team fallback"));
    }
}

// FUN_140696300: only in Lobby (Battle_WaitForPlayers is battle-only).
void UFDSLobbyAuthority::OnSetIsReadyInLobby(AHypePlayerState* Player, bool bIsReady)
{
    const EMultiplayerGameplayStage Type = CurrentStageType();
    if (Type != EMultiplayerGameplayStage::Lobby && Type != EMultiplayerGameplayStage::Battle_WaitForPlayers)
    {
        return;
    }
    const uint8 Bits = bIsReady ? (ReadyBits(Player) | ReadyLobbyBit) : (ReadyBits(Player) & ~ReadyLobbyBit);
    Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(Bits);
    Player->ForceNetUpdate();
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d ready in lobby: %s"), Player->PlayerId, bIsReady ? TEXT("yes") : TEXT("no"));
    UpdateEveryoneReady();
}

// FUN_140696370: Results_XPPayout only, so this only matters with ResultsXPPayout=True (otherwise Results goes
// straight back to the Lobby).
void UFDSLobbyAuthority::OnSetIsReadyInResults(AHypePlayerState* Player, bool bIsReady)
{
    if (CurrentStageType() != EMultiplayerGameplayStage::Results_XPPayout)
    {
        return;
    }
    const uint8 Bits = bIsReady ? (ReadyBits(Player) | ReadyResultsBit) : (ReadyBits(Player) & ~ReadyResultsBit);
    Player->_isReadyCheck = static_cast<EHypePlayerStateIsReadyFlags>(Bits);
    Player->ForceNetUpdate();
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d ready in results: %s"), Player->PlayerId, bIsReady ? TEXT("yes") : TEXT("no"));
    UpdateEveryoneReady();
}

// FUN_14062bee0: Lobby only; joining the DJs is refused when the queue is full.
void UFDSLobbyAuthority::OnRequestTeamChange(AHypePlayerState* Player, EMultiplayerTeam NewTeam)
{
    if (bMainStage || CurrentStageType() != EMultiplayerGameplayStage::Lobby || Player->_team == NewTeam)
    {
        return;
    }
    if (NewTeam == EMultiplayerTeam::DJ && DJQueue.Num() >= MaxNumDJs)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d team change to DJ refused: %d DJ slots taken"), Player->PlayerId, DJQueue.Num());
        return;
    }
    SetTeam(Player, NewTeam);
    AssignDJIndices();
    RebuildDJQueue();
    UpdateEveryoneReady();
    LogPlayers(TEXT("team change"));
}

// FUN_14062bac0 through the leader's RPC (HypeServerRpcs.cpp checks the leader).
void UFDSLobbyAuthority::OnMarkLobbyComplete(AHypePlayerState* Player)
{
    if (bMainStage)
    {
        return; // the event schedule starts Main Stage sets
    }
    CompleteLobby(TEXT("the leader's ServerMarkLobbyComplete"));
}

// The DJ's master tempo (ServerUpdateGlobalAudioState, empty in FUSER's shipping build). FUSER's dedicated server
// applied it through its audio engine; without it the server's clock keeps the Lobby tempo and every client is pulled
// back every 20-60 s. FDS re-anchors its clock at once, on arrival.
// Only during a set, from a DJ, and not from another DJ during someone's turn.
void UFDSLobbyAuthority::OnUpdateGlobalAudioState(AHypePlayerState* Player, int32 Tick, int32 BPM, EKey Key, EKeyMode Mode)
{
    const FMultiplayerGameStage* S = Stage(CurrentStage);
    if (!S || CurrentStage <= 0 || Player->_team != EMultiplayerTeam::DJ || (S->djIndex != -1 && S->djIndex != Player->_djIndex))
    {
        return;
    }
    if (BPM < 40 || BPM > 300)
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: player %d tempo %d BPM ignored (out of range)"), Player->PlayerId, BPM);
        return;
    }
    if (BPM != ClockBpm)
    {
        SetClockBpm(BPM);
    }
    MasterKey = Key;
    MasterMode = Mode;
    if (BPM != LastLoggedBpm)
    {
        LastLoggedBpm = BPM;
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d tempo %d BPM (stamp %d, master tick %d, bar %d, key %d, mode %d)"), Player->PlayerId, BPM, Tick, ClockTick(),
            ClockMeasure(), static_cast<int32>(Key), static_cast<int32>(Mode));
    }
}

// ServerSetCoopRoundNumTurnMeasures, empty in FUSER's shipping build: its dedicated server took the
// set length from the first player who sends it (a public ticket's turn length isn't reliable). Only in the Lobby before its countdown; later calls are ignored.
void UFDSLobbyAuthority::OnSetCoopRoundNumTurnMeasures(AHypePlayerState* Player, int32 NumRounds, int32 TurnMeasures)
{
    const FMultiplayerGameStage* LobbyStage = Stage(CurrentStage);
    if (bMainStage || bSetLengthChosen || CurrentStageType() != EMultiplayerGameplayStage::Lobby || !LobbyStage || LobbyStage->ServerWorldTimeStageEnd != -1.0f)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: player %d set length %d x %d ignored (%s)"), Player->PlayerId, NumRounds, TurnMeasures,
            bMainStage ? TEXT("Main Stage") : bSetLengthChosen ? TEXT("already chosen") : TEXT("not in an open Lobby"));
        return;
    }
    if (NumRounds < 1 || NumRounds > 10 || TurnMeasures < 8 || TurnMeasures > 512)
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: player %d set length %d x %d out of range"), Player->PlayerId, NumRounds, TurnMeasures);
        return;
    }
    bSetLengthChosen = true;
    CoopRounds = NumRounds;
    CoopTurnMeasures = TurnMeasures;
    if (UMultiplayerGameplayState* M = Mps())
    {
        M->_numCoopFreestyleRounds = CoopRounds;
        M->_coopTurnLengthMeasures = CoopTurnMeasures;
        BuildStages(); // the Lobby stays stage 0; the turns follow the new length
        for (AHypePlayerState* Each : Players())
        {
            ResetResults(Each); // the per-measure tables are sized by rounds x turn measures
        }
    }
    if (GameState.IsValid())
    {
        GameState->ForceNetUpdate();
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: set length %d round(s) of %d measures (player %d's choice)"), CoopRounds, CoopTurnMeasures, Player->PlayerId);
}

FFDSLobbyStatus UFDSLobbyAuthority::GetStatus() const
{
    FFDSLobbyStatus Status;
    for (AHypePlayerState* Player : Players())
    {
        ++Status.Players;
        (Player->_team == EMultiplayerTeam::DJ ? Status.DJs : Status.Viewers)++;
    }
    Status.Stage = static_cast<int32>(CurrentStageType());
    Status.Round = RoundIndex;
    Status.Rounds = CoopRounds;
    Status.TurnMeasures = CoopTurnMeasures;
    for (AHypePlayerState* Player : Players())
    {
        const int32 Id = bMainStage ? FDSMainStage::PinnedUserId(Cast<APlayerController>(Player->GetOwner())) : Player->_hypeUserId;
        if (Id > 0)
        {
            Status.Members.Emplace(Id, Player->_team);
        }
    }
    if (CurrentDJ.IsValid())
    {
        Status.CurrentDJ = bMainStage ? FDSMainStage::PinnedUserId(Cast<APlayerController>(CurrentDJ->GetOwner())) : CurrentDJ->_hypeUserId;
        Status.CurrentDJ = FMath::Max(Status.CurrentDJ, 0);
    }
    if (bMainStage && CurrentStage > 0)
    {
        Status.Event = StartedEvent;
    }
    return Status;
}

void UFDSLobbyAuthority::OnChangeLeaderTo(AHypePlayerState* Caller, AHypePlayerState* NewLeader)
{
    if (bMainStage)
    {
        return;
    }
    SetSessionLeader(NewLeader);
    LogPlayers(TEXT("leader change"));
}

// FUN_140696b40 (freestyle branch): when AFK checks are on, a player whose reported idle time reaches the threshold
// for the session's co-op turn length (MPS +0x2a8) is kicked by the game session with reason "afk_dj". FUSER applies
// it to every reporting player, without a team or stage check. In the Main Stage mode the threshold is the Main
// Stage one (Start: settings +0x13c/+0x140, AfkSecondsTillKickMainStageSpectator/Performer, 1800 s).
void UFDSLobbyAuthority::OnUpdateAfkSeconds(AHypePlayerController* Controller, int32 AfkSeconds)
{
    const UMultiplayerGameplayState* M = Mps();
    const int32* Mapped = M ? AfkSecondsTillKick.Find(M->_coopTurnLengthMeasures) : nullptr;
    const int32 Threshold = Mapped ? *Mapped : AfkSecondsTillKickDefault;
    UE_LOG(LogFDS, Verbose, TEXT("FDS: AFK report %d s (kick at %d s)"), AfkSeconds, Threshold);
    if (!bEnableAfkChecks || AfkSeconds < Threshold || !Controller || Controller->IsPendingKillPending())
    {
        return;
    }
    AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
    AGameSession* Session = GameMode ? GameMode->GameSession : nullptr;
    if (Session)
    {
        const APlayerState* Player = Controller->PlayerState;
        UE_LOG(LogFDS, Display, TEXT("FDS: kicking player %d: idle %d s, threshold %d s (afk_dj)"), Player ? Player->PlayerId : -1, AfkSeconds, Threshold);
        Session->KickPlayer(Controller, FText::FromString(TEXT("afk_dj")));
    }
}

// FUN_140695ac0 (the RPC body checks the leader): the target's controller is kicked by the game session with
// reason "kicked_by_server_leader". FUSER does not stop a leader kicking themselves; neither does FDS.
void UFDSLobbyAuthority::OnLeaderKickPlayer(AHypePlayerState* Leader, AHypePlayerState* Target)
{
    APlayerController* Controller = Cast<APlayerController>(Target->GetOwner());
    AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
    AGameSession* Session = GameMode ? GameMode->GameSession : nullptr;
    if (!Controller || !Session || Controller->IsPendingKillPending())
    {
        return;
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: leader player %d kicks player %d (kicked_by_server_leader)"), Leader->PlayerId, Target->PlayerId);
    Session->KickPlayer(Controller, FText::FromString(TEXT("kicked_by_server_leader")));
}

// FUN_140696880: any player's request flips EnableAfkChecks for the whole server, as in FUSER.
void UFDSLobbyAuthority::OnToggleAfkChecks(AHypePlayerState* Player)
{
    bEnableAfkChecks = !bEnableAfkChecks;
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d toggled AFK checks: now %s"), Player->PlayerId, bEnableAfkChecks ? TEXT("on") : TEXT("off"));
}

// FUN_140696010: only while the current stage is Lobby (FUN_1406243c0 reads _gameStages[current].Stage == 1), the
// game state multicasts the sender's signature move to every client (FUN_140829470 -> NetMulticastSignatureMove).
void UFDSLobbyAuthority::OnSendSignatureMove(AHypePlayerState* Player)
{
    if (!GameState.IsValid() || CurrentStageType() != EMultiplayerGameplayStage::Lobby)
    {
        return;
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d signature move"), Player->PlayerId);
    GameState->NetMulticastSignatureMove(Player);
}

void UFDSLobbyAuthority::LogPlayers(const TCHAR* Why) const
{
    FString Text;
    for (AHypePlayerState* Player : Players())
    {
        Text += FString::Printf(TEXT("%s[player %d: %s, dj index %d%s%s]"), Text.IsEmpty() ? TEXT("") : TEXT(" "), Player->PlayerId,
            TeamName(Player->_team), Player->_djIndex, Player->_isSessionLeader ? TEXT(", leader") : TEXT(""),
            (ReadyBits(Player) & ReadyLobbyBit) ? TEXT(", ready") : TEXT(""));
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: players after %s: %d DJ queue, %d spectators %s"), Why, DJQueue.Num(), Spectators.Num(), *Text);
}

// ---------------------------------------------------------------------------------------------- Main Stage

// MPS slot 148 (FUN_140636900): only in the Lobby, a player whose hype user id is among the next event's headliners
// (FUN_140631f40) is a DJ; everyone else spectates. The id is the one fuserd's pass admitted (FDSMainStage), never the client's
// own claim, so a player still waiting for the check spectates.
EMultiplayerTeam UFDSLobbyAuthority::MainStageTeam(const AHypePlayerState* Player) const
{
    return CurrentStageType() == EMultiplayerGameplayStage::Lobby && HeadlinerRank(Player) != MAX_int32 ? EMultiplayerTeam::DJ
                                                                                                       : EMultiplayerTeam::Spectator;
}

int32 UFDSLobbyAuthority::HeadlinerRank(const AHypePlayerState* Player) const
{
    const FFDSMainStageEvent* Event = PlacementEvent();
    if (!Player || !Event || Event->Type == TEXT("maintenance"))
    {
        return MAX_int32;
    }
    const int32 Id = FDSMainStage::PinnedUserId(Cast<APlayerController>(Player->GetOwner()));
    const int32 Rank = Id == INDEX_NONE ? INDEX_NONE : Event->Headliners.IndexOfByKey(Id);
    return Rank == INDEX_NONE ? MAX_int32 : Rank;
}

// The event whose headliners are placed as DJs in the Lobby: the next event (stock FUN_140631f40), or the
// current event when it has not had its set yet and still has room for one (late start: the server restarted during
// it, its headliners arrived after its start, or the previous set overran it). Room: about two minutes of the other
// stages plus a 32-bar turn at the lobby tempo.
const FFDSMainStageEvent* UFDSLobbyAuthority::PlacementEvent(bool* bLate) const
{
    const FFDSMainStageSchedule& Schedule = FDSMainStage::Schedule();
    const double UtcNow = FDSMainStage::UtcNow();
    if (bLate)
    {
        *bLate = false;
    }
    if (Schedule.Current.IsSet() && Schedule.Current->Uuid != StartedEvent && Schedule.Current->Type != TEXT("maintenance") &&
        Schedule.Current->End - UtcNow >= 120.0 + TurnBlockBars * 240.0 / FMath::Max(ClockBpm, 40))
    {
        if (bLate)
        {
            *bLate = true;
        }
        return &Schedule.Current.GetValue();
    }
    return Schedule.Next.IsSet() && Schedule.Next->Start > UtcNow ? &Schedule.Next.GetValue() : nullptr;
}

// The Main Stage's lobby, twice a second: the server side of MPS slots 149 (FUN_140636a60: every player's team from the
// next event's headliners, then the DJ indices) and 145 (FUN_140638110: when the next event starts within
// MinLobbyReadyTimer and DJs are queued, everyone is ready). The set then starts as a freestyle one does at lobby
// completion, with one round and the event's turn length (turn minutes x BPM / 4 bars, whole
// phrases, so that every turn fits in the event). An event without a headliner present gets no set.
void UFDSLobbyAuthority::UpdateMainStage()
{
    if (!bMainStage || CurrentStageType() != EMultiplayerGameplayStage::Lobby || Now() < NextMainStageUpdate)
    {
        return;
    }
    NextMainStageUpdate = Now() + 0.5f;
    const FMultiplayerGameStage* LobbyStage = Stage(CurrentStage);
    if (LobbyStage && LobbyStage->ServerWorldTimeStageEnd != -1.0f)
    {
        return; // the set's countdown runs: its DJs stay put although their event has now become the current one
    }
    const FFDSMainStageSchedule& Schedule = FDSMainStage::Schedule();
    if (!Schedule.bReceived && FFDSRegistration::Get().IsEnabled() && Now() > 30.0f)
    {
        static bool bWarned = false;
        if (!bWarned)
        {
            bWarned = true;
            UE_LOG(LogFDS, Warning, TEXT("FDS: main stage: no schedule from fuserd 30 s after start: nobody DJs until one arrives (does fuserd send main_stage?)"));
        }
    }
    PruneDeparted();
    bool bLate = false;
    const FFDSMainStageEvent* Next = PlacementEvent(&bLate);
    if (Next && Next->Type != TEXT("maintenance"))
    {
        MaxNumDJs = FMath::Clamp(Next->Slots, 1, 8);
    }
    bool bChanged = false;
    for (AHypePlayerState* Player : Players())
    {
        if (PendingTeam.Contains(Player) || Player->_team == EMultiplayerTeam::Unknown)
        {
            continue; // placed once its hype user id arrives (OnSetHypeUserId / AssignPendingTeams)
        }
        const EMultiplayerTeam Team = MainStageTeam(Player);
        if (Player->_team != Team)
        {
            SetTeam(Player, Team);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        AssignDJIndices();
        RebuildDJQueue();
        LogPlayers(TEXT("main stage headliners"));
    }
    if (!Next || Next->Uuid == StartedEvent || (!bLate && Next->Start - FDSMainStage::UtcNow() > MinLobbyReadyTimer))
    {
        LateFirstDJAt = -1.0f;
        return;
    }
    if (Next->Type == TEXT("maintenance"))
    {
        StartedEvent = Next->Uuid;
        UE_LOG(LogFDS, Display, TEXT("FDS: main stage: maintenance %s begins: no set"), *Next->Uuid);
        return;
    }
    if (DJQueue.Num() == 0)
    {
        // Not marked as played: a headliner who arrives while the event still has room starts it late.
        if (NoHeadlinersLogged != Next->Uuid)
        {
            NoHeadlinersLogged = Next->Uuid;
            UE_LOG(LogFDS, Display, TEXT("FDS: main stage: event %s starts with none of its %d headliner(s) here: no set unless one arrives in time"),
                *Next->Uuid, Next->Headliners.Num());
        }
        LateFirstDJAt = -1.0f;
        return;
    }
    if (bLate)
    {
        // A late start waits for the event's other headliners: all of them here, or LateStartGraceSeconds after the
        // first was placed (teams are fixed once the set starts).
        constexpr float LateStartGraceSeconds = 20.0f;
        if (LateFirstDJAt < 0.0f)
        {
            LateFirstDJAt = Now();
            UE_LOG(LogFDS, Display, TEXT("FDS: main stage: event %s is running without its set: a late start once its headliners are here (at most %.0f s)"),
                *Next->Uuid, LateStartGraceSeconds);
        }
        if (DJQueue.Num() < Next->Headliners.Num() && Now() - LateFirstDJAt < LateStartGraceSeconds)
        {
            return;
        }
    }
    StartedEvent = Next->Uuid;
    LateFirstDJAt = -1.0f;
    // One turn per present headliner, the event's time split evenly
    // between them after about 120 s of the other stages, at the set's tempo, in whole 32-bar blocks (rounded down):
    // 1750 s gives 288 bars (9 x 32) for 3 headliners and 192 (6 x 32) for 4, as FUSER's Main Stage did. A turn is at
    // most TotalMeasuresForMainStageDJPerformances (640) bars (assumed; only a lone headliner reaches it), and at most the
    // event's turn minutes when fuserd gives some (0 = no limit).
    const int32 Bpm = FMath::Max(ClockBpm, 40);
    const double PerTurnSeconds = (Next->End - FMath::Max(Next->Start, FDSMainStage::UtcNow()) - 120.0) / DJQueue.Num(); // late: the time left
    const int32 Fits = FMath::Max(1, FMath::FloorToInt(PerTurnSeconds * Bpm / 240.0 / TurnBlockBars)) * TurnBlockBars;
    const int32 Limit = Next->TurnMinutes > 0
        ? FMath::Max(1, FMath::FloorToInt(Next->TurnMinutes * Bpm / 4.0f / TurnBlockBars)) * TurnBlockBars : MainStageMaxTurnBars;
    const int32 Bars = FMath::Clamp(FMath::Min(Fits, Limit), TurnBlockBars, MainStageMaxTurnBars);
    const int32 Wanted = Fits; // for the log
    UMultiplayerGameplayState* M = Mps();
    CoopRounds = 1;
    CoopTurnMeasures = Bars;
    M->_numCoopFreestyleRounds = CoopRounds;
    M->_coopTurnLengthMeasures = CoopTurnMeasures;
    BuildStages(); // the Lobby stays stage 0; the turns follow the event's layout
    for (AHypePlayerState* Each : Players())
    {
        ResetResults(Each); // the per-measure tables are sized by rounds x turn measures
    }
    AssignDJIndices();
    RebuildDJQueue();
    UE_LOG(LogFDS, Display, TEXT("FDS: main stage: event %s starts in %.1f s%s: %d DJ(s) of %d headliner(s), turns of %d bars at %d BPM (%d fit, limit %d)"),
        *Next->Uuid, Next->Start - FDSMainStage::UtcNow(), bLate ? TEXT(" (late start)") : TEXT(""), DJQueue.Num(), Next->Headliners.Num(), Bars, Bpm,
        Wanted, Limit);
    bEveryoneReady = true; // slot 145: the start time readies the lobby, not the players
    CompleteLobby(TEXT("main stage event start"));
}

// Every ServerMainStageRhythmojiProcessingTime (1 s) the reactions received since the last update become the
// per-second rate of each praise type (the unit a client's own FUN_140695330 uses: count / elapsed seconds). The
// replicated array is only written when it changes, so an idle crowd costs nothing.
void UFDSLobbyAuthority::UpdateRhythmojiRates()
{
    UMultiplayerGameplayState* M = Mps();
    if (!bMainStage || !M)
    {
        return;
    }
    if (RhythmojiWindowStart < 0.0f)
    {
        RhythmojiWindowStart = Now();
        return;
    }
    const float Elapsed = Now() - RhythmojiWindowStart;
    if (Elapsed < RhythmojiProcessingTime)
    {
        return;
    }
    RhythmojiWindowStart = Now();
    // Test hook: -FDSMainStageTestPraises=<per second> makes the first spectator send that
    // many Fire reactions to the turn's DJ each window, as a FUSER client would. Never set in normal runs.
    static const int32 TestPraises = []
    {
        int32 Value = 0;
        FParse::Value(FCommandLine::Get(), TEXT("FDSMainStageTestPraises="), Value);
        return Value;
    }();
    if (TestPraises > 0 && CurrentDJ.IsValid() && CurrentStageType() == EMultiplayerGameplayStage::Playing)
    {
        for (AHypePlayerState* Player : Players())
        {
            if (Player->_team == EMultiplayerTeam::Spectator)
            {
                TArray<FCollectedPraise> Praises;
                for (int32 i = 0; i < FMath::RoundToInt(TestPraises * Elapsed); ++i)
                {
                    FCollectedPraise& Praise = Praises.AddDefaulted_GetRef();
                    Praise.ForPlayer = CurrentDJ.Get();
                    Praise.FromPlayer = Player;
                    Praise.Measure = ClockMeasure();
                    Praise.praiseType = EMultiplayerPraiseType::Fire;
                }
                OnSendPraises(Player, Praises);
                break;
            }
        }
    }
    const int32 Types = static_cast<int32>(EMultiplayerPraiseType::Num);
    RhythmojiCounts.SetNumZeroed(Types);
    TArray<float> Rates;
    Rates.SetNumZeroed(Types);
    int32 Total = 0;
    for (int32 i = 0; i < Types; ++i)
    {
        Rates[i] = RhythmojiCounts[i] / Elapsed;
        Total += RhythmojiCounts[i];
        RhythmojiCounts[i] = 0;
    }
    if (Rates != M->MainStageRhythmojiRates)
    {
        M->MainStageRhythmojiRates = Rates;
        if (GameState.IsValid())
        {
            GameState->ForceNetUpdate();
        }
        static float NextCrowdLog = 0.0f;
        if (Total > 0 && Now() >= NextCrowdLog)
        {
            NextCrowdLog = Now() + 30.0f;
            UE_LOG(LogFDS, Display, TEXT("FDS: main stage crowd: %d reaction(s) in %.1f s (%.1f per second); rates sent"), Total, Elapsed, Total / Elapsed);
        }
    }
}

// A Main Stage client takes its results screen's hottest mix and praise results from ResultsState
// ._mainStageResults (FUN_14064e3e0 copies it into _results' MyHottestMix and PraiseResults). FDS keeps scoring into
// _results as in freestyle and copies those two parts over when Results begins.
void UFDSLobbyAuthority::PublishMainStageResults()
{
    int32 Published = 0;
    for (AHypePlayerState* Player : Players())
    {
        if (UResultsState* Results = Player->_resultsState)
        {
            Results->_mainStageResults.MyHottestMix = Results->_results.MyHottestMix;
            Results->_mainStageResults.PraiseResults = Results->_results.PraiseResults;
            Player->ForceNetUpdate();
            ++Published;
        }
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: main stage results published for %d player(s)"), Published);
}

// The Main Stage server runs for days, so the per-player maps lose players who left: praise tallies and pending
// crafted-instrument requests are keyed by weak pointers that would otherwise pile up.
void UFDSLobbyAuthority::PruneDeparted()
{
    for (auto It = PraiseGiven.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
    for (auto It = MostPraised.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
    for (auto It = PendingCrafted.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
