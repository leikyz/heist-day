#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "WebSocketsModule.h"
#include "Json.h"

bool UEOSMatchmakingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer();
}

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
    // Load WebSocket Module if not already loaded
    FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");

    // Connect to Go Backend
    FString ServerURL = TEXT("ws://127.0.0.1:8080/api/matchmake");
    MatchmakingSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

    // Bind Events
    MatchmakingSocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketConnected);
    MatchmakingSocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketMessage);
    MatchmakingSocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketError);
    MatchmakingSocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketClosed);

    OnMatchmakingStatusChanged.Broadcast(TEXT("Connecting to Matchmaker..."));
    MatchmakingSocket->Connect();
}

void UEOSMatchmakingSubsystem::OnSocketConnected()
{
    OnMatchmakingStatusChanged.Broadcast(TEXT("In Queue. Waiting for opponent..."));
    UE_LOG(LogTemp, Warning, TEXT("CLIENT: Connected to Go Matchmaker!"));
}

void UEOSMatchmakingSubsystem::OnSocketMessage(const FString& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("CLIENT: Received WS Message: %s"), *Message);

    // Parse the JSON from the Go Server
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString Status;
        if (JsonObject->TryGetStringField("status", Status) && Status == "found")
        {
            FString ServerIP;
            if (JsonObject->TryGetStringField("ip", ServerIP))
            {
                OnMatchmakingStatusChanged.Broadcast(TEXT("Match Found! Traveling..."));

                // Teleport the player to the Dedicated Server
                if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
                {
                    PC->ClientTravel(ServerIP, TRAVEL_Absolute);
                }
            }
        }
    }
}

void UEOSMatchmakingSubsystem::OnSocketError(const FString& Error)
{
    OnMatchmakingStatusChanged.Broadcast(TEXT("Matchmaking Error."));
    UE_LOG(LogTemp, Error, TEXT("CLIENT WS ERROR: %s"), *Error);
}

void UEOSMatchmakingSubsystem::OnSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Warning, TEXT("CLIENT WS CLOSED: %s"), *Reason);
}