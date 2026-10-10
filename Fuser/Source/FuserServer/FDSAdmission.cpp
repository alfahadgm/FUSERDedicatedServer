#include "FDSAdmission.h"
#include "FDSMainStage.h"
#include "FDSSessionToken.h"
#include "FuserServer.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

namespace
{
    bool GInitialized = false;
    bool GRequired = false;
    FString GSlotId;
    TMap<FString, TWeakObjectPtr<AController>> GLive; // token player session (psid) -> the connection holding it
    TSet<FString> GAdmitted; // every psid this server has admitted: its token stays good here for a reconnect

    // Verifies the login's token; on success fills the claims.
    FString VerifyOptions(const FString& Options, FDSSessionToken::FClaims& Claims)
    {
        const FString Token = UGameplayStatics::ParseOption(Options, TEXT("playerSessionId"));
        if (Token.IsEmpty())
        {
            return TEXT("no playerSessionId");
        }
        const FString Why = FDSSessionToken::Verify(Token, FDateTime::UtcNow().ToUnixTimestamp(), Claims);
        // Expiry bounds the first join only. A player who drops after it reconnects with the token they joined with
        // (Verify fills the claims of a correctly signed, expired token).
        const bool bReconnect = Why == TEXT("expired") && GAdmitted.Contains(Claims.PlayerSession);
        if (!Why.IsEmpty() && !bReconnect)
        {
            return Why;
        }
        if (Claims.Slot != GSlotId)
        {
            return TEXT("token for another server");
        }
        const FString User = UGameplayStatics::ParseOption(Options, TEXT("hypeUserId"));
        if (!User.IsEmpty() && FCString::Atoi64(*User) != Claims.User)
        {
            return TEXT("token for another user");
        }
        const FString GameSession = UGameplayStatics::ParseOption(Options, TEXT("gameSessionId"));
        if (!GameSession.IsEmpty() && GameSession != Claims.GameSession)
        {
            return TEXT("token for another game session");
        }
        return FString();
    }
}

void FDSAdmission::Init()
{
    if (GInitialized)
    {
        return;
    }
    GInitialized = true;
    GSlotId = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_SLOT_ID")).TrimStartAndEnd();
    const FString Key = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_TOKEN_PUBLIC_KEY"));
    const FString Require = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_REQUIRE_TOKEN"));
    const bool bKey = !Key.IsEmpty() && FDSSessionToken::SetPublicKey(Key);
    if (!Key.IsEmpty() && !bKey)
    {
        UE_LOG(LogFDS, Error, TEXT("FDS: FDS_TOKEN_PUBLIC_KEY is not a 32-byte base64url key"));
    }
    GRequired = Require == TEXT("1") || (Require != TEXT("0") && !Key.IsEmpty());
    if (FDSMainStage::IsEnabled())
    {
        // Main Stage joins carry no session token (FUSER clears it): fuserd passes are checked instead (FDSMainStage).
        GRequired = false;
        UE_LOG(LogFDS, Display, TEXT("FDS: admission by fuserd main stage pass (no session tokens)"));
        return;
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: admission %s"), GRequired
        ? *FString::Printf(TEXT("requires fuserd session tokens for slot %s%s"), GSlotId.IsEmpty() ? TEXT("(none)") : *GSlotId,
            bKey ? TEXT("") : TEXT(" - but there is no usable key: every client will be refused"))
        : TEXT("open (no FDS_TOKEN_PUBLIC_KEY): any client may join"));
}

bool FDSAdmission::IsRequired()
{
    return GRequired;
}

FString FDSAdmission::Check(const FString& Options)
{
    if (!GRequired)
    {
        return FString();
    }
    if (GSlotId.IsEmpty())
    {
        return TEXT("no FDS_SLOT_ID");
    }
    FDSSessionToken::FClaims Claims;
    const FString Why = VerifyOptions(Options, Claims);
    if (!Why.IsEmpty())
    {
        return Why;
    }
    if (const TWeakObjectPtr<AController>* Holder = GLive.Find(Claims.PlayerSession))
    {
        if (Holder->IsValid())
        {
            return TEXT("token already in use by a connected player");
        }
    }
    return FString();
}

void FDSAdmission::Admitted(const FString& Options, APlayerController* Player)
{
    FDSSessionToken::FClaims Claims;
    if (GRequired && Player && VerifyOptions(Options, Claims).IsEmpty())
    {
        GLive.Add(Claims.PlayerSession, Player);
        GAdmitted.Add(Claims.PlayerSession);
    }
}

void FDSAdmission::Left(AController* Player)
{
    for (auto It = GLive.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid() || It.Value().Get() == Player)
        {
            It.RemoveCurrent();
        }
    }
}
