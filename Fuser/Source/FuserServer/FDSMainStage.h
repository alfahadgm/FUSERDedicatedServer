#pragma once

#include "CoreMinimal.h"

class APlayerController;
class FJsonObject;

// One Main Stage event as fuserd sends it in the heartbeat reply, times in this machine's UTC seconds (fuserd's
// clock offset applied). Headliners are the booked players' hype user ids in slot order: the DJ order.
struct FFDSMainStageEvent
{
    FString Uuid;
    FString Type; // event, special_event, maintenance
    double Start = 0.0;
    double End = 0.0;
    int32 Slots = 0;
    int32 TurnMinutes = 0;
    TArray<int32> Headliners;
};

struct FFDSMainStageSchedule
{
    bool bReceived = false;
    TOptional<FFDSMainStageEvent> Current;
    TOptional<FFDSMainStageEvent> Next;
};

// The Main Stage mode: one long-running server, spawned by fuserd as its persistent "mainstage"
// slot, that clients reach through main_stage/servers/ instead of a ticket. Selected by -FDSMainStage or
// FDS_MAIN_STAGE=1.
// Admission: FUSER's Main Stage join clears the player session id (_OnGetMainStageInfo,
// FUN_14071ce70), so no session token arrives. fuserd instead records a pass when a logged-in player asks
// main_stage/servers/ for the address; after login FDS asks fuserd (POST /fds/v1/mainstage/admit) whether the login's
// hypeUserId holds a pass from the connecting address, kicks the player when not, and pins the id so a later
// ServerSetHypeUserId can't change it. Without fuserd registration (local runs) every login is admitted.
namespace FDSMainStage
{
    bool IsEnabled();

    // After login: checks the player's pass with fuserd and kicks on refusal or no answer. Returns false when the login
    // has no hype user id (kicked at once).
    bool CheckPass(APlayerController* Player, int32 HypeUserId);

    // Every frame: kicks players whose pass check got no answer in time.
    void Tick();

    // The hype user id fuserd admitted for this player, or INDEX_NONE while the check is pending / not run.
    int32 PinnedUserId(const APlayerController* Player);
    void Forget(const APlayerController* Player);

    // The schedule: fuserd's heartbeat reply carries it (FFDSRegistration). Local runs without fuserd can give a
    // test event: -FDSMainStageTestEvent=<seconds from start>,<turn minutes>,<hype id>+<hype id>...
    double UtcNow();
    const FFDSMainStageSchedule& Schedule();
    void ReceiveSchedule(const TSharedPtr<FJsonObject>& MainStage);
}
