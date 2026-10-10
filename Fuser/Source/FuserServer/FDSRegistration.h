#pragma once

#include "CoreMinimal.h"
#include "FDSLobbyAuthority.h"
#include "Interfaces/IHttpRequest.h"

class UWorld;

// A container fuserd starts registers with fuserd's /fds/v1 API and reports its
// status, instead of fuserd reading its log.
//   FDS_REGISTER_URL      fuserd's base URL (host networking), FDS_SLOT_ID the slot fuserd started this container for
//   FDS_REGISTER_TOKEN    the slot's bearer token: environment only, never logged
//   FDS_HEARTBEAT_SECONDS status interval (default 10)
//   FDS_EXIT_WHEN_EMPTY   0 keeps the server running after its last player leaves (default: exit; the Main Stage never
//                         exits when empty)
// Without FDS_REGISTER_URL / FDS_SLOT_ID / FDS_REGISTER_TOKEN nothing is sent (local runs).
class FFDSRegistration
{
public:
    static FFDSRegistration& Get();
    ~FFDSRegistration();

    bool IsEnabled() const { return bEnabled; }

    // The game mode is up and listening: register (retried until fuserd accepts).
    void Start(UWorld* World);
    // Every frame: retries, heartbeats and status changes; the exit when the last player has left.
    void Tick(float DeltaSeconds, const FFDSLobbyStatus& Status);
    // Shutdown (docker stop, or the server's own exit): report it, waiting briefly for the reply.
    void ReportExit(const FString& Reason);

    // fuserd's game session id for this server, once a ticket completed on it (empty until then).
    const FString& SessionId() const { return SessionIdValue; }

    // The slot id fuserd started this container for.
    const FString& Slot() const { return SlotId; }
    // Another /fds/v1 call (the Main Stage pass check, FDSMainStage): POSTs Body to /fds/v1/<Call> with the slot's token;
    // Done gets the HTTP status (0 = no reply) and the reply body. Independent of the heartbeat's request.
    void Post(const TCHAR* Call, const FString& Body, TFunction<void(int32, const FString&)> Done);

private:
    FFDSRegistration();
    void Send(const TCHAR* Call, const FString& Body);
    void OnReply(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk, FString Call);
    FString RegistrationBody() const;
    FString StatusBody(const FFDSLobbyStatus& Status) const;

    bool bEnabled = false;
    bool bExitWhenEmpty = true;
    FString Url, SlotId, Token, Version;
    float HeartbeatSeconds = 10.0f;
    TWeakObjectPtr<UWorld> World;

    bool bStarted = false;
    bool bRegistered = false;
    bool bExitSent = false;
    bool bInFlight = false;
    double NextSendAt = 0.0;
    FFDSLobbyStatus Last;
    bool bHadPlayers = false;
    FString SessionIdValue;
};
