#include "FDSMainStage.h"
#include "FDSRegistration.h"
#include "FuserServer.h"
#include "Dom/JsonObject.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    constexpr double PassTimeoutSeconds = 15.0;

    struct FPending
    {
        double Deadline = 0.0;
        int32 UserId = INDEX_NONE;
    };
    TMap<TWeakObjectPtr<APlayerController>, FPending> GPending;
    TMap<TWeakObjectPtr<APlayerController>, int32> GPinned;
    FFDSMainStageSchedule GSchedule;

    TOptional<FFDSMainStageEvent> ParseEvent(const TSharedPtr<FJsonObject>& Json, double OffsetSeconds)
    {
        TOptional<FFDSMainStageEvent> Out;
        if (!Json.IsValid())
        {
            return Out;
        }
        FFDSMainStageEvent Event;
        double StartMs = 0.0, EndMs = 0.0;
        Json->TryGetStringField(TEXT("uuid"), Event.Uuid);
        Json->TryGetStringField(TEXT("type"), Event.Type);
        Json->TryGetNumberField(TEXT("start_ms"), StartMs);
        Json->TryGetNumberField(TEXT("end_ms"), EndMs);
        Json->TryGetNumberField(TEXT("slots"), Event.Slots);
        Json->TryGetNumberField(TEXT("turn_minutes"), Event.TurnMinutes);
        const TArray<TSharedPtr<FJsonValue>>* Ids = nullptr;
        if (Json->TryGetArrayField(TEXT("headliners"), Ids))
        {
            for (const TSharedPtr<FJsonValue>& Id : *Ids)
            {
                Event.Headliners.Add(static_cast<int32>(Id->AsNumber()));
            }
        }
        if (Event.Uuid.IsEmpty() || EndMs <= StartMs)
        {
            return Out;
        }
        Event.Start = StartMs / 1000.0 - OffsetSeconds;
        Event.End = EndMs / 1000.0 - OffsetSeconds;
        Out = Event;
        return Out;
    }

    FString Describe(const TOptional<FFDSMainStageEvent>& Event)
    {
        if (!Event.IsSet())
        {
            return TEXT("none");
        }
        const FFDSMainStageEvent& E = Event.GetValue();
        return FString::Printf(TEXT("%s %s in %.0f s for %.0f s, %d/%d headliner(s), %d-minute turns"), *E.Type, *E.Uuid,
            E.Start - FDSMainStage::UtcNow(), E.End - E.Start, E.Headliners.Num(), E.Slots, E.TurnMinutes);
    }

    // -FDSMainStageTestEvent (local runs only): one event, the given seconds after the first call.
    void MaybeTestEvent()
    {
        static bool bDone = false;
        FString Spec;
        if (bDone || GSchedule.bReceived || !FParse::Value(FCommandLine::Get(), TEXT("FDSMainStageTestEvent="), Spec))
        {
            return;
        }
        bDone = true;
        TArray<FString> Parts;
        Spec.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() < 2)
        {
            return;
        }
        FFDSMainStageEvent Event;
        Event.Uuid = TEXT("00000000-0000-4000-8000-00000000test");
        Event.Type = TEXT("event");
        Event.Start = FDSMainStage::UtcNow() + FCString::Atod(*Parts[0]);
        Event.End = Event.Start + 1750.0;
        Event.Slots = 3;
        Event.TurnMinutes = FCString::Atoi(*Parts[1]);
        if (Parts.Num() > 2)
        {
            TArray<FString> Ids;
            Parts[2].ParseIntoArray(Ids, TEXT("+"));
            for (const FString& Id : Ids)
            {
                Event.Headliners.Add(FCString::Atoi(*Id));
            }
        }
        GSchedule.bReceived = true;
        GSchedule.Next = Event;
        UE_LOG(LogFDS, Display, TEXT("FDS: main stage test event: %s"), *Describe(GSchedule.Next));
    }

    void Kick(APlayerController* Player, const TCHAR* Why)
    {
        UWorld* World = Player ? Player->GetWorld() : nullptr;
        AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
        if (!GameMode || !GameMode->GameSession || Player->IsPendingKillPending())
        {
            return;
        }
        UE_LOG(LogFDS, Warning, TEXT("FDS: main stage: kicking player %d: %s"), Player->PlayerState ? Player->PlayerState->PlayerId : -1, Why);
        GameMode->GameSession->KickPlayer(Player, FText::FromString(TEXT("invalid_player_session")));
    }
}

bool FDSMainStage::IsEnabled()
{
    static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("FDSMainStage")) ||
        FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_MAIN_STAGE")) == TEXT("1");
    return bEnabled;
}

bool FDSMainStage::CheckPass(APlayerController* Player, int32 HypeUserId)
{
    if (!Player)
    {
        return false;
    }
    if (HypeUserId <= 0)
    {
        Kick(Player, TEXT("no hype user id in the login"));
        return false;
    }
    FFDSRegistration& Registration = FFDSRegistration::Get();
    if (!Registration.IsEnabled())
    {
        static bool bLogged = false;
        if (!bLogged)
        {
            bLogged = true;
            UE_LOG(LogFDS, Warning, TEXT("FDS: main stage: no fuserd registration: passes are not checked, every login is admitted"));
        }
        GPinned.Add(Player, HypeUserId);
        return true;
    }
    UNetConnection* Connection = Player->GetNetConnection();
    const FString Address = Connection ? Connection->LowLevelGetRemoteAddress(false) : FString();
    GPending.Add(Player, FPending{FPlatformTime::Seconds() + PassTimeoutSeconds, HypeUserId});
    TWeakObjectPtr<APlayerController> Weak(Player);
    Registration.Post(TEXT("mainstage/admit"),
        FString::Printf(TEXT("{\"slot_id\":\"%s\",\"user_id\":%d,\"address\":\"%s\"}"), *Registration.Slot(), HypeUserId,
            *Address.Replace(TEXT("\""), TEXT(""))),
        [Weak, HypeUserId](int32 Code, const FString& Body)
        {
            APlayerController* PC = Weak.Get();
            if (!PC || !GPending.Contains(Weak))
            {
                return; // gone, or timed out and kicked already
            }
            GPending.Remove(Weak);
            bool bAdmit = false;
            FString Reason = Code ? FString::Printf(TEXT("HTTP %d"), Code) : TEXT("no reply");
            TSharedPtr<FJsonObject> Reply;
            if (Code == 200 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Reply) && Reply.IsValid())
            {
                Reply->TryGetBoolField(TEXT("admit"), bAdmit);
                Reply->TryGetStringField(TEXT("reason"), Reason);
            }
            if (!bAdmit)
            {
                Kick(PC, *FString::Printf(TEXT("fuserd refused the pass (%s)"), *Reason));
                return;
            }
            GPinned.Add(Weak, HypeUserId);
            UE_LOG(LogFDS, Display, TEXT("FDS: main stage: player %d admitted by fuserd pass"), PC->PlayerState ? PC->PlayerState->PlayerId : -1);
        });
    return true;
}

void FDSMainStage::Tick()
{
    const double Now = FPlatformTime::Seconds();
    for (auto It = GPending.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
        else if (Now >= It.Value().Deadline)
        {
            APlayerController* PC = It.Key().Get();
            It.RemoveCurrent();
            Kick(PC, TEXT("no answer from fuserd about the pass"));
        }
    }
}

int32 FDSMainStage::PinnedUserId(const APlayerController* Player)
{
    const int32* Id = GPinned.Find(const_cast<APlayerController*>(Player));
    return Id ? *Id : INDEX_NONE;
}

void FDSMainStage::Forget(const APlayerController* Player)
{
    GPinned.Remove(const_cast<APlayerController*>(Player));
    GPending.Remove(const_cast<APlayerController*>(Player));
}

double FDSMainStage::UtcNow()
{
    return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalSeconds();
}

const FFDSMainStageSchedule& FDSMainStage::Schedule()
{
    MaybeTestEvent();
    return GSchedule;
}

void FDSMainStage::ReceiveSchedule(const TSharedPtr<FJsonObject>& MainStage)
{
    if (!MainStage.IsValid())
    {
        return;
    }
    double ServerMs = 0.0;
    MainStage->TryGetNumberField(TEXT("server_time_ms"), ServerMs);
    const double Offset = ServerMs > 0.0 ? ServerMs / 1000.0 - UtcNow() : 0.0;
    const TSharedPtr<FJsonObject>* Current = nullptr;
    const TSharedPtr<FJsonObject>* Next = nullptr;
    MainStage->TryGetObjectField(TEXT("current"), Current);
    MainStage->TryGetObjectField(TEXT("next"), Next);
    FFDSMainStageSchedule Received;
    Received.bReceived = true;
    Received.Current = ParseEvent(Current ? *Current : nullptr, Offset);
    Received.Next = ParseEvent(Next ? *Next : nullptr, Offset);
    auto Same = [](const TOptional<FFDSMainStageEvent>& A, const TOptional<FFDSMainStageEvent>& B)
    {
        return A.IsSet() == B.IsSet() && (!A.IsSet() || (A->Uuid == B->Uuid && A->Headliners == B->Headliners &&
            FMath::Abs(A->Start - B->Start) < 1.0 && FMath::Abs(A->End - B->End) < 1.0 && A->TurnMinutes == B->TurnMinutes));
    };
    const bool bChanged = !GSchedule.bReceived || !Same(GSchedule.Current, Received.Current) || !Same(GSchedule.Next, Received.Next);
    GSchedule = Received;
    if (bChanged)
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: main stage schedule (fuserd clock %+.2f s): current %s; next %s"), Offset,
            *Describe(GSchedule.Current), *Describe(GSchedule.Next));
    }
}
