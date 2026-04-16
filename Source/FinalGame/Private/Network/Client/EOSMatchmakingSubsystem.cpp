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
	if (MatchmakingSocket.IsValid() && MatchmakingSocket->IsConnected())
	{
		MatchmakingSocket->Close();
	}

	FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");

	FString CleanLobbyId = LobbyIdString.Replace(TEXT(" "), TEXT("")).Replace(TEXT(":"), TEXT("_")).Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));

	FString ServerURL = FString::Printf(TEXT("ws://127.0.0.1:8080/api/party?lobbyId=%s"), *CleanLobbyId);
	MatchmakingSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

	MatchmakingSocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketConnected);
	MatchmakingSocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketMessage);
	MatchmakingSocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketError);
	MatchmakingSocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketClosed);

	UE_LOG(LogTemp, Warning, TEXT("Attempting to connect to Go Tracker at: %s"), *ServerURL);

	MatchmakingSocket->Connect();
}

void UEOSMatchmakingSubsystem::SendReadyState(bool bIsReady)
{
	if (MatchmakingSocket.IsValid() && MatchmakingSocket->IsConnected())
	{
		FString JsonString = bIsReady ? TEXT("{\"isReady\": true}") : TEXT("{\"isReady\": false}");
		MatchmakingSocket->Send(JsonString);
	}
}

void UEOSMatchmakingSubsystem::DisconnectTracker()
{
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

	MatchmakingSocket->OnConnected().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketConnected);
	MatchmakingSocket->OnMessage().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketMessage);
	MatchmakingSocket->OnConnectionError().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketError);
	MatchmakingSocket->OnClosed().AddUObject(this, &UEOSMatchmakingSubsystem::OnSocketClosed);

	OnMatchmakingStatusChanged.Broadcast(TEXT("Connecting to Matchmaker..."));
	MatchmakingSocket->Connect();
}

void UEOSMatchmakingSubsystem::OnSocketConnected()
{
	UE_LOG(LogTemp, Warning, TEXT("CLIENT: Connected to Go Server tracker!"));
}

void UEOSMatchmakingSubsystem::OnSocketMessage(const FString& Message)
{
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

void UEOSMatchmakingSubsystem::OnSocketError(const FString& Error)
{
	OnMatchmakingStatusChanged.Broadcast(TEXT("Connection Error."));
	UE_LOG(LogTemp, Error, TEXT("CLIENT WS ERROR: %s"), *Error);
}

void UEOSMatchmakingSubsystem::OnSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogTemp, Warning, TEXT("CLIENT WS CLOSED: %s"), *Reason);
}