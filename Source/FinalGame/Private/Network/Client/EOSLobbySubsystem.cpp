#include "Network/Client/EOSLobbySubsystem.h"
#include "Network/Client/EOSIdentitySubsystem.h"
#include <eos_presence_types.h>
#include "Online/UserInfo.h"
#include <Network/Client/EOSMatchmakingSubsystem.h>
using namespace UE::Online;

bool UEOSLobbySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UEOSLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineServicesPtr Services = GetServices();
	if (Services)
	{
		ILobbiesPtr Lobbies = Services->GetLobbiesInterface();
		if (Lobbies)
		{
			MemberJoinedHandle = Lobbies->OnLobbyMemberJoined().Add([this](const FLobbyMemberJoined& Event) {
				OnLobbyMemberJoined(Event);
				});

			MemberLeftHandle = Lobbies->OnLobbyMemberLeft().Add([this](const FLobbyMemberLeft& Event) {
				OnLobbyMemberLeft(Event);
				});

			UIJoinRequestedHandle = Lobbies->OnUILobbyJoinRequested().Add([this](const FUILobbyJoinRequested& Event) {
				OnLobbyJoinRequested(Event);
				});

			AttributesChangedHandle = Lobbies->OnLobbyAttributesChanged().Add([this](const UE::Online::FLobbyAttributesChanged& Event) {
				OnLobbyAttributesUpdate(Event);
				});

			MemberAttributesChangedHandle = Lobbies->OnLobbyMemberAttributesChanged().Add([this](const UE::Online::FLobbyMemberAttributesChanged& Event) {
				OnLobbyMemberAttributesUpdate(Event);
				});

			//Lobbies->OnLobbyLeft().Add([this](const FLobbyLeft& Event) {
			//	UE_LOG(LogTemp, Warning, TEXT("Local player has left the lobby. Resetting UI..."));

			//	// Reset our local variables
			//	CurrentLobbyId = UE::Online::FLobbyId();
			//	CurrentPlayerCount = 0;

			//	// Force the Blueprints to refresh and show "Empty" slots
			//	NotifyLobbyStateChanged();
			//	});
		}
	}
}
TArray<FLobbyPlayerInfo> UEOSLobbySubsystem::GetLobbyPlayersInfo()
{
	TArray<FLobbyPlayerInfo> PlayersInfo;

	if (CachedLobbyObject.IsValid())
	{
		IOnlineServicesPtr Services = GetServices();
		IUserInfoPtr UserInfoInterface = Services->GetUserInfoInterface();
		UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();

		if (UserInfoInterface && IdentitySubsystem)
		{
			UE::Online::FAccountId LocalUserId = IdentitySubsystem->GetLocalAccountId();

			// On récupère l'ID du Chef du Lobby
			UE::Online::FAccountId OwnerUserId = CachedLobbyObject->OwnerAccountId;

			for (const auto& Kvp : CachedLobbyObject->Members)
			{
				FLobbyPlayerInfo Info;
				Info.bIsReady = false;

				// On vérifie si ce membre est le Chef du Lobby
				Info.bIsLobbyLeader = (Kvp.Key == OwnerUserId);

				// 1. Récupération du pseudo
				FGetUserInfo::Params UserParams;
				UserParams.LocalAccountId = LocalUserId;
				UserParams.AccountId = Kvp.Key;

				TOnlineResult<FGetUserInfo> UserResult = UserInfoInterface->GetUserInfo(MoveTemp(UserParams));
				if (UserResult.IsOk())
				{
					Info.DisplayName = UserResult.GetOkValue().UserInfo->DisplayName;
				}
				else
				{
					FString PlayerID = UE::Online::ToLogString(Kvp.Key);
					Info.DisplayName = FString::Printf(TEXT("Joueur: %s"), *PlayerID);
				}

				// 2. Récupération du statut "Prêt"
				if (Kvp.Value->Attributes.Contains(FName(TEXT("IsReady"))))
				{
					Info.bIsReady = Kvp.Value->Attributes[FName(TEXT("IsReady"))].GetBoolean();
				}

				// 3. LE TRI PAR CHEF DE GROUPE
				if (Info.bIsLobbyLeader)
				{
					// Le Chef est TOUJOURS inséré au début du tableau (Index 0)
					PlayersInfo.Insert(Info, 0);
				}
				else
				{
					// Les invités sont ajoutés à la suite (Index 1, 2, etc.)
					PlayersInfo.Add(Info);
				}
			}
		}
	}

	return PlayersInfo;
}
void UEOSLobbySubsystem::OnLobbyAttributesUpdate(const UE::Online::FLobbyAttributesChanged& Event)
{
	FName QueueStateKey(TEXT("QueueState"));
	if (Event.AddedAttributes.Contains(QueueStateKey))
	{
		OnLobbyMatchmakingStateSync.Broadcast(Event.AddedAttributes[QueueStateKey].GetString());
	}
	else if (Event.ChangedAttributes.Contains(QueueStateKey))
	{
		OnLobbyMatchmakingStateSync.Broadcast(Event.ChangedAttributes[QueueStateKey].Value.GetString());
	}

	FName ServerIPKey(TEXT("ServerIP"));
	FString TargetIP = TEXT("");

	if (Event.AddedAttributes.Contains(ServerIPKey))
	{
		TargetIP = Event.AddedAttributes[ServerIPKey].GetString();
	}
	else if (Event.ChangedAttributes.Contains(ServerIPKey))
	{
		TargetIP = Event.ChangedAttributes[ServerIPKey].Value.GetString();
	}

	if (!TargetIP.IsEmpty())
	{
		FString PartyIDString = UE::Online::ToLogString(CurrentLobbyId);

		FString TravelURL = FString::Printf(TEXT("%s?PartyID=%s"), *TargetIP, *PartyIDString);

		UE_LOG(LogTemp, Warning, TEXT("Lobby Broadcast reçu ! Téléportation de l'équipe vers le serveur : %s"), *TravelURL);

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ClientTravel(TravelURL, TRAVEL_Absolute);
		}
	}
}

void UEOSLobbySubsystem::OnLobbyJoinRequested(const UE::Online::FUILobbyJoinRequested& Event)
{
	if (Event.Result.IsOk())
	{
		TSharedRef<const UE::Online::FLobby> TargetLobby = Event.Result.GetOkValue();
		UE_LOG(LogTemp, Log, TEXT("Overlay requested to join lobby: %s"), *ToLogString(TargetLobby->LobbyId));

		if (IsInLobby())
		{
			PendingLobbyId = TargetLobby->LobbyId;
			PendingAccountId = Event.LocalAccountId;
			bHasPendingJoin = true;

			UE_LOG(LogTemp, Warning, TEXT("Already in a lobby. Leaving current lobby before joining the new one..."));
			LeaveLobby();
		}
		else
		{
			ProcessJoinLobby(Event.LocalAccountId, TargetLobby->LobbyId);
		}
	}
	else
	{
		FString ErrorMessage = ToLogString(Event.Result.GetErrorValue());
		UE_LOG(LogTemp, Error, TEXT("Failed to join lobby via UI: %s"), *ErrorMessage);
	}
}

void UEOSLobbySubsystem::ProcessJoinLobby(UE::Online::FAccountId InLocalAccountId, UE::Online::FLobbyId InLobbyId)
{
	UE::Online::IOnlineServicesPtr Services = UE::Online::GetServices();
	if (Services)
	{
		UE::Online::ILobbiesPtr LobbiesInterface = Services->GetLobbiesInterface();
		if (LobbiesInterface)
		{
			UE::Online::FJoinLobby::Params JoinParams;
			JoinParams.LocalAccountId = InLocalAccountId;
			JoinParams.LobbyId = InLobbyId;
			JoinParams.bPresenceEnabled = true;

			LobbiesInterface->JoinLobby(MoveTemp(JoinParams)).OnComplete(
				[this](const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& JoinResult)
				{
					OnJoinLobbyComplete(JoinResult);
				}
			);
		}
	}
}

void UEOSLobbySubsystem::OnJoinLobbyComplete(const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& Result)
{
	if (Result.IsOk())
	{
		const TSharedPtr<const UE::Online::FLobby>& Lobby = Result.GetOkValue().Lobby;

		if (Lobby.IsValid())
		{
			CurrentLobbyId = Lobby->LobbyId;
			CurrentPlayerCount = Lobby->Members.Num();

			UE_LOG(LogTemp, Log, TEXT("Successfully joined the lobby! %s. Players in lobby: %d"),
				*ToLogString(Lobby->LobbyId), CurrentPlayerCount);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Successfully Joined Lobby!"));
		}

		CachedLobbyObject = Result.GetOkValue().Lobby; 
		CurrentLobbyId = CachedLobbyObject->LobbyId;
		CurrentPlayerCount = CachedLobbyObject->Members.Num();

		NotifyLobbyStateChanged();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to join lobby: %s"),
			*Result.GetErrorValue().GetLogString());
	}
}

void UEOSLobbySubsystem::OnLobbyMemberJoined(const UE::Online::FLobbyMemberJoined& Event)
{
	CurrentPlayerCount++;
	FString JoinMsg = FString::Printf(TEXT("A player joined! Current Count: %d"), CurrentPlayerCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, JoinMsg);
	}

	CachedLobbyObject = Event.Lobby; 
	CurrentPlayerCount = CachedLobbyObject->Members.Num();
	NotifyLobbyStateChanged();
}

void UEOSLobbySubsystem::Deinitialize()
{
	MemberJoinedHandle.Unbind();
	MemberLeftHandle.Unbind();
	UIJoinRequestedHandle.Unbind();
	AttributesChangedHandle.Unbind(); 
	MemberAttributesChangedHandle.Unbind();

	Super::Deinitialize();
}

void UEOSLobbySubsystem::CreateLobby()
{
	UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	if (!IdentitySubsystem || !IdentitySubsystem->IsLoggedIn()) return;

	IOnlineServicesPtr Services = GetServices();
	if (!Services || !Services->GetLobbiesInterface()) return;

	FCreateLobby::Params Params;
	Params.LocalAccountId = IdentitySubsystem->GetLocalAccountId();
	Params.LocalName = TEXT("MatchmakingLobby"); // Local name for the handle
	Params.SchemaId = FName(TEXT("GameLobby"));
	Params.MaxMembers = 2; // FIX: Set to 2 for 1v1 matchmaking
	Params.bPresenceEnabled = true;
	Params.JoinPolicy = UE::Online::ELobbyJoinPolicy::PublicAdvertised;

	// --- CRITICAL FIX: Add the attribute so other clients can find this lobby ---
	FSchemaVariant BucketValue(TEXT("MainMatchmaking"));
	Params.Attributes.Add(FName(TEXT("BucketId")), BucketValue);

	UE_LOG(LogTemp, Warning, TEXT("EOSLobbySubsystem: Creating Matchmaking Lobby with BucketId..."));

	Services->GetLobbiesInterface()->CreateLobby(MoveTemp(Params)).OnComplete(
		[this](const UE::Online::TOnlineResult<UE::Online::FCreateLobby>& Result)
		{
			OnCreateLobbyComplete(Result);
		}
	);
}

bool UEOSLobbySubsystem::IsLobbyLeader() const
{
	if (!CachedLobbyObject.IsValid()) return false;
	UEOSIdentitySubsystem* Identity = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	return CachedLobbyObject->OwnerAccountId == Identity->GetLocalAccountId();
}

void UEOSLobbySubsystem::SyncMatchmakingState(FString NewState)
{
	if (!IsInLobby() || !IsLobbyLeader()) return;

	UEOSIdentitySubsystem* Identity = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	IOnlineServicesPtr Services = UE::Online::GetServices();

	UE::Online::FModifyLobbyAttributes::Params Params;
	Params.LocalAccountId = Identity->GetLocalAccountId();
	Params.LobbyId = CurrentLobbyId;
	Params.UpdatedAttributes.Add(FName(TEXT("QueueState")), UE::Online::FSchemaVariant(NewState));

	Services->GetLobbiesInterface()->ModifyLobbyAttributes(MoveTemp(Params));
}

void UEOSLobbySubsystem::OnCreateLobbyComplete(const TOnlineResult<FCreateLobby>& Result)
{
	if (Result.IsOk())
	{
		// LIGNE CORRIGÉE : On sauvegarde l'objet du lobby !
		CachedLobbyObject = Result.GetOkValue().Lobby;

		CurrentLobbyId = CachedLobbyObject->LobbyId;
		CurrentPlayerCount = CachedLobbyObject->Members.Num();

		UE_LOG(LogTemp, Warning, TEXT("EOSLobbySubsystem: Lobby créé avec succès ! ID: %s"), *UE::Online::ToLogString(CurrentLobbyId));
		NotifyLobbyStateChanged();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EOSLobbySubsystem: Échec création Lobby: %s"), *Result.GetErrorValue().GetLogString());
	}
}

void UEOSLobbySubsystem::LeaveLobby()
{
	if (!IsInLobby()) return;

	UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	IOnlineServicesPtr Services = GetServices();

	if (Services && IdentitySubsystem)
	{
		FLeaveLobby::Params Params;
		Params.LocalAccountId = IdentitySubsystem->GetLocalAccountId();
		Params.LobbyId = CurrentLobbyId;

		Services->GetLobbiesInterface()->LeaveLobby(MoveTemp(Params)).OnComplete(
			[this](const UE::Online::TOnlineResult<UE::Online::FLeaveLobby>& Result)
			{
				OnLeaveLobbyComplete(Result);
			}
		);


	}
}

void UEOSLobbySubsystem::StartGame(FString ServerIP)
{
	UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	UE::Online::IOnlineServicesPtr Services = UE::Online::GetServices();

	if (Services && IdentitySubsystem && IsInLobby())
	{
		UE::Online::FModifyLobbyAttributes::Params Params;
		Params.LocalAccountId = IdentitySubsystem->GetLocalAccountId();
		Params.LobbyId = CurrentLobbyId;

		// Add the Server IP to the Lobby Attributes
		Params.UpdatedAttributes.Add(FName(TEXT("ServerIP")), UE::Online::FSchemaVariant(ServerIP));

		UE_LOG(LogTemp, Log, TEXT("Leader is broadcasting Server IP to lobby..."));

		Services->GetLobbiesInterface()->ModifyLobbyAttributes(MoveTemp(Params)).OnComplete(
			[this](const UE::Online::TOnlineResult<UE::Online::FModifyLobbyAttributes>& Result)
			{
				if (Result.IsOk())
				{
					// We DO NOT ClientTravel here anymore! 
					// EOS will trigger OnLobbyAttributesUpdate for EVERYONE (including the leader),
					// so we let that function handle the travel for everyone safely.
					UE_LOG(LogTemp, Log, TEXT("Successfully broadcasted the Server IP!"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to broadcast Server IP: %s"), *Result.GetErrorValue().GetLogString());
				}
			}
		);
	}
}

void UEOSLobbySubsystem::OnLeaveLobbyComplete(const TOnlineResult<FLeaveLobby>& Result)
{
	CurrentLobbyId = UE::Online::FLobbyId();
	CurrentPlayerCount = 0;
	CachedLobbyObject.Reset();
	NotifyLobbyStateChanged();

	OnLeaveLobbyFinished.Broadcast(Result.IsOk());

	UE_LOG(LogTemp, Log, TEXT("Successfully left the lobby."));	

	if (bHasPendingJoin)
	{
		bHasPendingJoin = false;
		UE_LOG(LogTemp, Log, TEXT("Executing pending join request..."));
		ProcessJoinLobby(PendingAccountId, PendingLobbyId);
	}
}

bool UEOSLobbySubsystem::IsInLobby() const
{
	return CurrentLobbyId.IsValid();
}

int32 UEOSLobbySubsystem::GetLobbyPlayerCount() const
{
	return CurrentPlayerCount;
}

void UEOSLobbySubsystem::OnLobbyMemberLeft(const FLobbyMemberLeft& Event)
{
	if (Event.Lobby->LobbyId == CurrentLobbyId)
	{
		CurrentPlayerCount--;
		FString PrintMsg = FString::Printf(TEXT("Un joueur est parti ! Reste %d joueurs."), CurrentPlayerCount);

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, PrintMsg);

		CachedLobbyObject = Event.Lobby;
		CurrentPlayerCount = CachedLobbyObject->Members.Num();
		NotifyLobbyStateChanged();
	}
}

void UEOSLobbySubsystem::NotifyLobbyStateChanged()
{
	OnLobbyStateChanged.Broadcast();
}

void UEOSLobbySubsystem::SetPlayerReady(bool bIsReady)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("C++: Bouton Prêt cliqué !"));

	if (!IsInLobby())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("C++: ERREUR - Le joueur n'est pas dans un lobby !"));
		return;
	}

	UEOSIdentitySubsystem* Identity = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	IOnlineServicesPtr Services = GetServices();

	UE::Online::FModifyLobbyMemberAttributes::Params Params;
	Params.LocalAccountId = Identity->GetLocalAccountId();
	Params.LobbyId = CurrentLobbyId;
	Params.UpdatedAttributes.Add(FName(TEXT("IsReady")), UE::Online::FSchemaVariant(bIsReady));

	Services->GetLobbiesInterface()->ModifyLobbyMemberAttributes(MoveTemp(Params)).OnComplete(
		[bIsReady](const UE::Online::TOnlineResult<UE::Online::FModifyLobbyMemberAttributes>& Result)
		{
			if (Result.IsOk())
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("C++: Serveur Epic a validé le statut !"));
			}
			else
			{
				FString ErrorMsg = Result.GetErrorValue().GetLogString();
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("C++: Refus d'Epic : %s"), *ErrorMsg));
			}
		}
	);
}
void UEOSLobbySubsystem::OnLobbyMemberAttributesUpdate(const UE::Online::FLobbyMemberAttributesChanged& Event)
{
	UE_LOG(LogTemp, Warning, TEXT("EOS Backend : Un membre a mis à jour ses attributs ! Mise à jour de l'UI..."));

	CachedLobbyObject = Event.Lobby;
	OnPlayerReadyUpdate.Broadcast();

	if (IsLobbyLeader())
	{
		CheckAllPlayersReady();
	}
}
void UEOSLobbySubsystem::CheckAllPlayersReady()
{
	if (!CachedLobbyObject.IsValid()) return;

	int32 TotalPlayers = CachedLobbyObject->Members.Num();
	int32 ReadyCount = 0;

	for (const auto& Kvp : CachedLobbyObject->Members)
	{
		if (Kvp.Value->Attributes.Contains(FName(TEXT("IsReady"))))
		{
			if (Kvp.Value->Attributes[FName(TEXT("IsReady"))].GetBoolean())
			{
				ReadyCount++;
			}
		}
	}

	if (ReadyCount == TotalPlayers && TotalPlayers > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tous les joueurs sont prêts ! Lancement auto du Matchmaking..."));

		if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
		{
			MatchSub->StartMatchmaking();
		}
	}
}