#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "Network/Client/EOSLobbySubsystem.h"
#include "WebSocketsModule.h"
#include "Json.h"

bool UEOSMatchmakingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer();
}

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
	if (MatchmakingSocket.IsValid() && MatchmakingSocket->IsConnected())
	{
		return;
	}

    UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>();
    int32 PartySize = 1;

    // If in a lobby, only the Leader is allowed to queue
    if (LobbySub && LobbySub->IsInLobby())
    {
        if (!LobbySub->IsLobbyLeader())
        {
            UE_LOG(LogTemp, Warning, TEXT("Only the Lobby Leader can start matchmaking."));
            return;
        }
        PartySize = LobbySub->GetLobbyPlayerCount();

        // Sync the UI for the non-leader!
        LobbySub->SyncMatchmakingState(TEXT("Searching for Match..."));
    }

    FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");

    // NEW: Append the PartySize to the URL
    FString ServerURL = FString::Printf(TEXT("ws://127.0.0.1:8080/api/matchmake?partySize=%d"), PartySize);
    MatchmakingSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

    MatchmakingSocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketConnected);
    MatchmakingSocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketMessage);
    MatchmakingSocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketError);
    MatchmakingSocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketClosed);

    OnMatchmakingStatusChanged.Broadcast(TEXT("Connecting to Matchmaker..."));
    MatchmakingSocket->Connect();
}

void UEOSMatchmakingSubsystem::OnSocketConnected()
{
	FString StateMsg = TEXT("In Queue. Waiting for opponent...");

	OnMatchmakingStatusChanged.Broadcast(StateMsg);
	UE_LOG(LogTemp, Warning, TEXT("CLIENT: Connected to Go Matchmaker!"));

	if (UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>())
	{
		LobbySub->SyncMatchmakingState(StateMsg);
	}
}

void UEOSMatchmakingSubsystem::OnSocketMessage(const FString& Message)
{
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
				UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>();

				if (LobbySub && LobbySub->IsInLobby() && LobbySub->IsLobbyLeader())
				{
					// TEAM TRAVEL: The leader broadcasts the IP to the lobby
					OnMatchmakingStatusChanged.Broadcast(TEXT("Match Found! Traveling Team..."));
					LobbySub->StartGame(ServerIP);
				}
				else
				{
					// SOLO TRAVEL: If no lobby, just travel instantly
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

void UEOSMatchmakingSubsystem::OnSocketError(const FString& Error)
{
    OnMatchmakingStatusChanged.Broadcast(TEXT("Matchmaking Error."));
    UE_LOG(LogTemp, Error, TEXT("CLIENT WS ERROR: %s"), *Error);
}

void UEOSMatchmakingSubsystem::OnSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Warning, TEXT("CLIENT WS CLOSED: %s"), *Reason);
}