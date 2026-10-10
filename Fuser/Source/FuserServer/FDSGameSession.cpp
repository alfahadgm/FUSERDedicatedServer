#include "FDSGameSession.h"
#include "FuserServer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/OnlineEngineInterface.h"

bool AFDSGameSession::IsDefaultSubsystemId(const FUniqueNetId& Id)
{
    return Id.GetType() == UOnlineEngineInterface::Get()->GetDefaultOnlineSubsystemName();
}

void AFDSGameSession::RegisterPlayer(APlayerController* NewPlayer, const TSharedPtr<const FUniqueNetId>& UniqueId, bool bWasFromInvite)
{
    if (!NewPlayer || !UniqueId.IsValid() || IsDefaultSubsystemId(*UniqueId))
    {
        Super::RegisterPlayer(NewPlayer, UniqueId, bWasFromInvite);
        return;
    }
    check(NewPlayer->PlayerState);
    NewPlayer->PlayerState->PlayerId = GetNextPlayerID();
    NewPlayer->PlayerState->SetUniqueId(UniqueId);
    UE_LOG(LogFDS, Display, TEXT("FDS: player %d registered without online-subsystem session (id type %s)"),
        NewPlayer->PlayerState->PlayerId, *UniqueId->GetType().ToString());
}

void AFDSGameSession::UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl& UniqueId)
{
    if (UniqueId.IsValid() && !IsDefaultSubsystemId(*UniqueId))
    {
        return; // never registered with the subsystem
    }
    Super::UnregisterPlayer(InSessionName, UniqueId);
}

void AFDSGameSession::UnregisterPlayers(FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& Players)
{
    TArray<TSharedRef<const FUniqueNetId>> Registered;
    for (const TSharedRef<const FUniqueNetId>& Id : Players)
    {
        if (IsDefaultSubsystemId(*Id))
        {
            Registered.Add(Id);
        }
    }
    Super::UnregisterPlayers(InSessionName, Registered);
}
