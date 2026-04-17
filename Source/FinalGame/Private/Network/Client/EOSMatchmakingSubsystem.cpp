#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "Network/Client/EOSLobbySubsystem.h"
#include "WebSocketsModule.h"
#include "Json.h"

bool UEOSMatchmakingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer();
}

void UEOSMatchmakingSubsystem::ConnectToPartyTracker(FString LobbyIdString)
{
    if (PartySocket.IsValid() && PartySocket->IsConnected())
    {
        PartySocket->Close();
    }

    FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");

    FString CleanLobbyId = LobbyIdString.Replace(TEXT(" "), TEXT("")).Replace(TEXT(":"), TEXT("_")).Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));

    FString ServerURL = FString::Printf(TEXT("ws://127.0.0.1:8080/api/party?lobbyId=%s"), *CleanLobbyId);
    PartySocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

    PartySocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnPartySocketConnected);
    PartySocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnPartySocketMessage);
    PartySocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnPartySocketError);
    PartySocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnPartySocketClosed);

    UE_LOG(LogTemp, Warning, TEXT("Attempting to connect to Go Party Tracker at: %s"), *ServerURL);

    PartySocket->Connect();
}

void UEOSMatchmakingSubsystem::SendReadyState(bool bIsReady)
{
    // Send ready state through the PartySocket, NOT the MatchmakingSocket
    if (PartySocket.IsValid() && PartySocket->IsConnected())
    {
        FString JsonString = bIsReady ? TEXT("{\"isReady\": true}") : TEXT("{\"isReady\": false}");
        PartySocket->Send(JsonString);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot send ready state: PartySocket is not connected."));
    }
}

void UEOSMatchmakingSubsystem::DisconnectTracker()
{
    if (PartySocket.IsValid() && PartySocket->IsConnected())
    {
        PartySocket->Close();
        PartySocket.Reset();
    }

    if (MatchmakingSocket.IsValid() && MatchmakingSocket->IsConnected())
    {
        MatchmakingSocket->Close();
        MatchmakingSocket.Reset();
    }
}

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
    if (MatchmakingSocket.IsValid() && MatchmakingSocket->IsConnected()) return;

    FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");
    FString ServerURL = TEXT("ws://127.0.0.1:8080/api/matchmake?partySize=1");
    MatchmakingSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

    MatchmakingSocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingSocketConnected);
    MatchmakingSocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingSocketMessage);
    MatchmakingSocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingSocketError);
    MatchmakingSocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingSocketClosed);

    OnMatchmakingStatusChanged.Broadcast(TEXT("Connecting to Matchmaker..."));
    MatchmakingSocket->Connect();
}

// ---------------------------------------------------------
// REUSABLE JSON PARSER FOR BOTH SOCKETS
// ---------------------------------------------------------
void UEOSMatchmakingSubsystem::ProcessServerMessage(const FString& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("RAW MESSAGE FROM SERVER: %s"), *Message);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString EventType;
        if (JsonObject->TryGetStringField("event", EventType) && EventType == "ready_update")
        {
            int32 ReadyCount = 0;
            int32 TotalCount = 0;
            JsonObject->TryGetNumberField("ready", ReadyCount);
            JsonObject->TryGetNumberField("total", TotalCount);

            OnPartyReadyUpdate.Broadcast(ReadyCount, TotalCount);
        }

        FString Status;
        if (JsonObject->TryGetStringField("status", Status))
        {
            if (Status == "queued")
            {
                FString StateMsg = TEXT("In Queue. Waiting for opponent...");
                OnMatchmakingStatusChanged.Broadcast(StateMsg);

                if (UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>())
                {
                    LobbySub->SyncMatchmakingState(StateMsg);
                }
            }
            else if (Status == "found")
            {
                FString ServerIP;
                if (JsonObject->TryGetStringField("ip", ServerIP))
                {
                    UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>();

                    if (LobbySub && LobbySub->IsInLobby() && LobbySub->IsLobbyLeader())
                    {
                        OnMatchmakingStatusChanged.Broadcast(TEXT("Match Found! Traveling Team..."));
                        LobbySub->StartGame(ServerIP);
                    }
                    else if (!LobbySub || !LobbySub->IsInLobby())
                    {
                        OnMatchmakingStatusChanged.Broadcast(TEXT("Match Found! Traveling..."));
                        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
                        {
                            PC->ClientTravel(ServerIP, TRAVEL_Absolute);
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------
// PARTY SOCKET CALLBACKS
// ---------------------------------------------------------
void UEOSMatchmakingSubsystem::OnPartySocketConnected()
{
    UE_LOG(LogTemp, Warning, TEXT("CLIENT: Connected to Go Party Tracker!"));
}

void UEOSMatchmakingSubsystem::OnPartySocketMessage(const FString& Message)
{
    ProcessServerMessage(Message);
}

void UEOSMatchmakingSubsystem::OnPartySocketError(const FString& Error)
{
    UE_LOG(LogTemp, Error, TEXT("PARTY WS ERROR: %s"), *Error);
}

void UEOSMatchmakingSubsystem::OnPartySocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Warning, TEXT("PARTY WS CLOSED: %s"), *Reason);
}

// ---------------------------------------------------------
// MATCHMAKER SOCKET CALLBACKS (For Solo Direct Queuing)
// ---------------------------------------------------------
void UEOSMatchmakingSubsystem::OnMatchmakingSocketConnected()
{
    UE_LOG(LogTemp, Warning, TEXT("CLIENT: Connected to Solo Matchmaker!"));
}

void UEOSMatchmakingSubsystem::OnMatchmakingSocketMessage(const FString& Message)
{
    ProcessServerMessage(Message);
}

void UEOSMatchmakingSubsystem::OnMatchmakingSocketError(const FString& Error)
{
    OnMatchmakingStatusChanged.Broadcast(TEXT("Connection Error."));
    UE_LOG(LogTemp, Error, TEXT("MATCHMAKER WS ERROR: %s"), *Error);
}

void UEOSMatchmakingSubsystem::OnMatchmakingSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Warning, TEXT("MATCHMAKER WS CLOSED: %s"), *Reason);
}