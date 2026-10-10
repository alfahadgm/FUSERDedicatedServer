#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "FDSGameSession.generated.h"

// FUSER clients present "Hype" unique ids. FDS loads no Hype online subsystem and keeps no online-subsystem session
// (fuserd owns sessions), so ids of another type than the loaded default are assigned to the PlayerState but not
// registered with that subsystem; the stock path would hand them to OnlineSubsystemNull, which asserts on the type.
// Ids of the default type (matching-project test clients) keep the stock behaviour.
UCLASS()
class FUSERSERVER_API AFDSGameSession : public AGameSession
{
    GENERATED_BODY()

public:
    virtual void RegisterPlayer(APlayerController* NewPlayer, const TSharedPtr<const FUniqueNetId>& UniqueId, bool bWasFromInvite) override;
    virtual void UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl& UniqueId) override;
    virtual void UnregisterPlayers(FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& Players) override;

private:
    static bool IsDefaultSubsystemId(const FUniqueNetId& Id);
};
