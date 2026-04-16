#include "Network/Client/EOSLobbySubsystem.h"
#include "Network/Client/EOSIdentitySubsystem.h"
#include <eos_presence_types.h>
#include "Online/UserInfo.h"
#include "Network/Client/EOSMatchmakingSubsystem.h"

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
			UE::Online::FAccountId OwnerUserId = CachedLobbyObject->OwnerAccountId;

			for (const auto& Kvp : CachedLobbyObject->Members)
			{
				FLobbyPlayerInfo Info;
				Info.bIsReady = false;
				Info.bIsLobbyLeader = (Kvp.Key == OwnerUserId);

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

				if (Info.bIsLobbyLeader)
				{
					PlayersInfo.Insert(Info, 0);
				}
				else
				{
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

		UE_LOG(LogTemp, Warning, TEXT("Lobby Broadcast reçu ! Téléportation vers : %s"), *TravelURL);

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

		if (IsInLobby())
		{
			PendingLobbyId = TargetLobby->LobbyId;
			PendingAccountId = Event.LocalAccountId;
			bHasPendingJoin = true;
			LeaveLobby();
		}
		else
		{
			ProcessJoinLobby(Event.LocalAccountId, TargetLobby->LobbyId);
		}
	}
}

void UEOSLobbySubsystem::ProcessJoinLobby(UE::Online::FAccountId InLocalAccountId, UE::Online::FLobbyId InLobbyId)
{
	UE::Online::IOnlineServicesPtr Services = UE::Online::GetServices();
	if (Services && Services->GetLobbiesInterface())
	{
		UE::Online::FJoinLobby::Params JoinParams;
		JoinParams.LocalAccountId = InLocalAccountId;
		JoinParams.LobbyId = InLobbyId;
		JoinParams.bPresenceEnabled = true;

		Services->GetLobbiesInterface()->JoinLobby(MoveTemp(JoinParams)).OnComplete(
			[this](const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& JoinResult)
			{
				OnJoinLobbyComplete(JoinResult);
			}
		);
	}
}

void UEOSLobbySubsystem::OnJoinLobbyComplete(const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& Result)
{
	if (Result.IsOk())
	{
		CachedLobbyObject = Result.GetOkValue().Lobby;
		CurrentLobbyId = CachedLobbyObject->LobbyId;
		CurrentPlayerCount = CachedLobbyObject->Members.Num();

		NotifyLobbyStateChanged();

		if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
		{
			MatchSub->ConnectToPartyTracker(UE::Online::ToLogString(CachedLobbyObject->OwnerAccountId));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to join lobby: %s"), *Result.GetErrorValue().GetLogString());
	}
}

void UEOSLobbySubsystem::OnLobbyMemberJoined(const UE::Online::FLobbyMemberJoined& Event)
{
	CurrentPlayerCount++;
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

	Super::Deinitialize();
}

void UEOSLobbySubsystem::CreateLobby()
{
	UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	IOnlineServicesPtr Services = GetServices();

	if (!IdentitySubsystem || !IdentitySubsystem->IsLoggedIn() || !Services) return;

	FCreateLobby::Params Params;
	Params.LocalAccountId = IdentitySubsystem->GetLocalAccountId();
	Params.LocalName = TEXT("MatchmakingLobby");
	Params.SchemaId = FName(TEXT("GameLobby"));
	Params.MaxMembers = 2;
	Params.bPresenceEnabled = true;
	Params.JoinPolicy = UE::Online::ELobbyJoinPolicy::PublicAdvertised;

	FSchemaVariant BucketValue(TEXT("MainMatchmaking"));
	Params.Attributes.Add(FName(TEXT("BucketId")), BucketValue);

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
		CachedLobbyObject = Result.GetOkValue().Lobby;
		CurrentLobbyId = CachedLobbyObject->LobbyId;
		CurrentPlayerCount = CachedLobbyObject->Members.Num();

		NotifyLobbyStateChanged();

		if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
		{
			MatchSub->ConnectToPartyTracker(UE::Online::ToLogString(CachedLobbyObject->OwnerAccountId));
		}
	}
}
void UEOSLobbySubsystem::LeaveLobby()
{
	if (!IsInLobby()) return;

	UEOSIdentitySubsystem* IdentitySubsystem = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	IOnlineServicesPtr Services = GetServices();

	if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
	{
		MatchSub->DisconnectTracker();
	}

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
		Params.UpdatedAttributes.Add(FName(TEXT("ServerIP")), UE::Online::FSchemaVariant(ServerIP));

		Services->GetLobbiesInterface()->ModifyLobbyAttributes(MoveTemp(Params));
	}
}

void UEOSLobbySubsystem::OnLeaveLobbyComplete(const TOnlineResult<FLeaveLobby>& Result)
{
	CurrentLobbyId = UE::Online::FLobbyId();
	CurrentPlayerCount = 0;
	CachedLobbyObject.Reset();
	NotifyLobbyStateChanged();
	OnLeaveLobbyFinished.Broadcast(Result.IsOk());

	if (bHasPendingJoin)
	{
		bHasPendingJoin = false;
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
	if (!IsInLobby()) return;

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("C++: Bouton Prêt cliqué (Bypass via WebSockets) !"));

	if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
	{
		MatchSub->SendReadyState(bIsReady);
	}
}