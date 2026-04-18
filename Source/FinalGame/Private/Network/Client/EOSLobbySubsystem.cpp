#include "Network/Client/EOSLobbySubsystem.h"
#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/World.h"

void UEOSLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->OnSessionSettingsUpdatedDelegates.AddUObject(this, &UEOSLobbySubsystem::OnSessionSettingsUpdated);
	}
}

void UEOSLobbySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

IOnlineSessionPtr UEOSLobbySubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		return Subsystem->GetSessionInterface();
	}
	return nullptr;
}

void UEOSLobbySubsystem::CreateLobby()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = 4; // Max players in your lobby
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true; // Use EOS backend Lobbies


	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UEOSLobbySubsystem::OnCreateSessionComplete);
	SessionInterface->CreateSession(0, NAME_GameSession, Settings);
}

void UEOSLobbySubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface) SessionInterface->ClearOnCreateSessionCompleteDelegates(this);

	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel("/Game/Level/Lobby_Level?listen");
		}
	}
}
void UEOSLobbySubsystem::FindAndJoinLobby()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UEOSLobbySubsystem::OnFindSessionsComplete);
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UEOSLobbySubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface) SessionInterface->ClearOnFindSessionsCompleteDelegates(this);

	if (bWasSuccessful && SessionSearch->SearchResults.Num() > 0)
	{
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UEOSLobbySubsystem::OnJoinSessionComplete);
		SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]);
	}
}

void UEOSLobbySubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface) SessionInterface->ClearOnJoinSessionCompleteDelegates(this);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectInfo))
		{
			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
			}
		}
	}
}

void UEOSLobbySubsystem::LeaveLobby()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UEOSLobbySubsystem::OnDestroySessionComplete);
	SessionInterface->DestroySession(NAME_GameSession);
}
void UEOSLobbySubsystem::NotifyLobbyStateChanged()
{
	OnLobbyStateChanged.Broadcast();
}
void UEOSLobbySubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface) SessionInterface->ClearOnDestroySessionCompleteDelegates(this);

	NotifyLobbyStateChanged();
	OnLeaveLobbyFinished.Broadcast(bWasSuccessful);

	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel("/Game/Level/Lobby_Level", TRAVEL_Absolute);
	}
}

bool UEOSLobbySubsystem::IsInLobby() const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface)
	{
		return SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
	}
	return false;
}

int32 UEOSLobbySubsystem::GetLobbyPlayerCount() const
{
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->PlayerArray.Num();
		}
	}
	return 0;
}

bool UEOSLobbySubsystem::IsLobbyLeader() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return PC->HasAuthority();
		}
	}
	return false;
}

void UEOSLobbySubsystem::SyncMatchmakingState(FString NewState)
{
	if (!IsLobbyLeader()) return;

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface)
	{
		FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(NAME_GameSession);
		if (Settings)
		{
			Settings->Set(FName("QueueState"), NewState, EOnlineDataAdvertisementType::ViaOnlineService);
			SessionInterface->UpdateSession(NAME_GameSession, *Settings, true);
		}
	}
}

void UEOSLobbySubsystem::OnSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& UpdatedSettings)
{
	FString NewState;
	if (UpdatedSettings.Get(FName("QueueState"), NewState))
	{
		OnLobbyMatchmakingStateSync.Broadcast(NewState);
	}
}

void UEOSLobbySubsystem::StartGame(FString ServerIP)
{
	if (IsLobbyLeader())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(ServerIP);
		}
	}
}

TArray<FLobbyPlayerInfo> UEOSLobbySubsystem::GetLobbyPlayersInfo()
{
	TArray<FLobbyPlayerInfo> PlayersInfo;
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
			IOnlineIdentityPtr IdentityInterface = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;

			for (APlayerState* PS : GameState->PlayerArray)
			{
				FLobbyPlayerInfo Info;

				Info.DisplayName = PS->GetPlayerName();

				if (IdentityInterface.IsValid() && PS->GetUniqueId().IsValid())
				{
					FString RealEpicName = IdentityInterface->GetPlayerNickname(*PS->GetUniqueId().GetUniqueNetId());

					if (!RealEpicName.IsEmpty())
					{
						Info.DisplayName = RealEpicName;
					}
				}

				Info.bIsLobbyLeader = (PS == GameState->PlayerArray[0]);

				Info.bIsReady = bIsReadyLocal;

				PlayersInfo.Add(Info);
			}
		}
	}
	return PlayersInfo;
}

void UEOSLobbySubsystem::SetPlayerReady(bool bIsReady)
{
	bIsReadyLocal = bIsReady;
	OnPlayerReadyUpdate.Broadcast();
}