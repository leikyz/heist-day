#include "Network/Client/EOSLobbySubsystem.h"
#include "Network/Client/EOSIdentitySubsystem.h"
#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "FindSessionsCallbackProxy.h"
#include "OnlineSubsystemUtils.h"
#include "Async/Async.h"

const FName UEOSLobbySubsystem::LobbySessionName = NAME_GameSession;

//  Lifecycle

void UEOSLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid()) return;

	OnSessionUserInviteAcceptedHandle = Sessions->OnSessionUserInviteAcceptedDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionUserInviteAccepted);
	OnSessionParticipantJoinedHandle = Sessions->OnSessionParticipantJoinedDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionParticipantJoined);
	OnSessionParticipantLeftHandle = Sessions->OnSessionParticipantLeftDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionParticipantLeft);
	OnSessionSettingsUpdatedHandle = Sessions->AddOnSessionSettingsUpdatedDelegate_Handle(
		FOnSessionSettingsUpdatedDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleSessionSettingsUpdated));

	if (IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr)
	{
		OnPresenceReceivedHandle = Presence->AddOnPresenceReceivedDelegate_Handle(FOnPresenceReceivedDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandlePresenceReceived));
	}
}

void UEOSLobbySubsystem::Deinitialize()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->OnSessionUserInviteAcceptedDelegates.Remove(OnSessionUserInviteAcceptedHandle);
		Sessions->OnSessionParticipantJoinedDelegates.Remove(OnSessionParticipantJoinedHandle);
		Sessions->OnSessionParticipantLeftDelegates.Remove(OnSessionParticipantLeftHandle);
		Sessions->ClearOnSessionSettingsUpdatedDelegate_Handle(OnSessionSettingsUpdatedHandle);
	}

	if (IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr)
	{
		Presence->ClearOnPresenceReceivedDelegate_Handle(OnPresenceReceivedHandle);
	}

	Super::Deinitialize();
}

//  Public API

void UEOSLobbySubsystem::CreateOwnLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid()) return;

	FOnlineSessionSettings Settings;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowInvites = true;
	Settings.NumPublicConnections = 4;

	OnCreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleCreateSessionComplete));
	Sessions->CreateSession(0, LobbySessionName, Settings);
}

void UEOSLobbySubsystem::JoinLobby(const FBlueprintSessionResult& SearchResult)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SearchResult.OnlineResult.IsValid()) return;

	if (Sessions->GetNamedSession(LobbySessionName))
	{
		bLeavingToJoin = true;
		PendingJoinResult = SearchResult;
		LeaveLobby();
		return;
	}

	OnJoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleJoinSessionComplete));

	TrackedUserIds.AddUnique(SearchResult.OnlineResult.Session.OwningUserId);

	Sessions->JoinSession(0, LobbySessionName, SearchResult.OnlineResult);
}

void UEOSLobbySubsystem::LeaveLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && Sessions->GetNamedSession(LobbySessionName))
	{
		OnDestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleDestroySessionComplete));
		Sessions->DestroySession(LobbySessionName);
	}
}

void UEOSLobbySubsystem::SetLocalPlayerReady(bool bReady)
{
	bLocalPlayerReady = bReady;
	UpdatePresenceData();
}

void UEOSLobbySubsystem::SyncMatchmakingState(const FString& StatusMessage)
{
	if (!IsLobbyLeader()) return;
	CachedMatchStatus = StatusMessage;
	UpdateLobbyGlobalData();
	RefreshMemberList();
}

void UEOSLobbySubsystem::BroadcastServerIP(const FString& ConnectionString)
{
	if (!IsLobbyLeader()) return;
	CachedServerIP = ConnectionString;
	UpdateLobbyGlobalData();
	RefreshMemberList();
}

//  Session Callbacks

void UEOSLobbySubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid()) Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionHandle);

	if (!bWasSuccessful) return;

	bIsInLobby = true;
	bIsOwner = true;

	if (UEOSIdentitySubsystem* ID = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>())
		TrackedUserIds.AddUnique(ID->GetLocalUserId().GetUniqueNetId());

	UpdatePresenceData();
}

void UEOSLobbySubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid()) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionHandle);

	if (Result != EOnJoinSessionCompleteResult::Success) return;

	bIsInLobby = true;
	bIsOwner = false;

	if (FNamedOnlineSession* Session = Sessions->GetNamedSession(LobbySessionName))
	{
		TrackedUserIds.AddUnique(Session->OwningUserId);
		for (const FUniqueNetIdPtr& Player : Session->RegisteredPlayers)
			TrackedUserIds.AddUnique(Player);
	}

	if (UEOSIdentitySubsystem* ID = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>())
		TrackedUserIds.AddUnique(ID->GetLocalUserId().GetUniqueNetId());

	UpdatePresenceData();
}

void UEOSLobbySubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid()) Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionHandle);

	// Destroy 3D models before flushing lobby data
	ClearLobbyAvatars();

	bIsInLobby = false;
	bIsOwner = false;
	bLocalPlayerReady = false;
	bIsMatchmakingStarted = false;
	bHasStartedTeleport = false;
	CurrentMembers.Empty();
	TrackedUserIds.Empty();
	CachedServerIP = TEXT("");
	CachedMatchStatus = TEXT("Idle");

	if (bLeavingToJoin)
	{
		bLeavingToJoin = false;
		JoinLobby(PendingJoinResult);
	}
	else
	{
		OnLobbyLeft.Broadcast();
	}
}
void UEOSLobbySubsystem::HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId)
{
	if (SessionName != LobbySessionName) return;
	TrackedUserIds.AddUnique(UniqueId.AsShared());
	RefreshMemberList();
}

void UEOSLobbySubsystem::HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason Reason)
{
	if (SessionName != LobbySessionName) return;
	TrackedUserIds.RemoveAll([&UniqueId](const FUniqueNetIdPtr& Id) { return *Id == UniqueId; });
	RefreshMemberList();
}

void UEOSLobbySubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful) return;
	FBlueprintSessionResult Res;
	Res.OnlineResult = InviteResult;
	JoinLobby(Res);
}

//  Presence Callbacks

void UEOSLobbySubsystem::HandleSetPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
	if (bIsOwner)
	{
		RefreshMemberList();
		if (CurrentMembers.Num() <= 1)
			OnLobbyCreated.Broadcast();
	}
	else if (bIsInLobby)
	{
		QueryPresenceForTrackedUsers();
	}
}

void UEOSLobbySubsystem::HandlePresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	RefreshMemberList();
}

//  Presence Query

void UEOSLobbySubsystem::QueryPresenceForTrackedUsers()
{
	IOnlinePresencePtr PresenceInterface = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
	UEOSIdentitySubsystem* ID = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	if (!PresenceInterface.IsValid() || !ID) return;

	TSharedPtr<const FUniqueNetId> LocalId = ID->GetLocalUserId().GetUniqueNetId();

	for (const FUniqueNetIdPtr& PeerId : TrackedUserIds)
	{
		if (!PeerId.IsValid() || (LocalId.IsValid() && *PeerId == *LocalId)) continue;

		// Correcting delegate binding for QueryPresence
		PresenceInterface->QueryPresence(*PeerId,
			IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleQueryPresenceComplete));
	}
}

void UEOSLobbySubsystem::HandleQueryPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
	RefreshMemberList();
	OnLobbyJoined.Broadcast();
}

//  Internal Helpers

void UEOSLobbySubsystem::UpdatePresenceData()
{
	IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
	UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();

	if (!Presence.IsValid() || !IdentitySub || !IdentitySub->IsLoggedIn()) return;

	TSharedPtr<const FUniqueNetId> LocalUserId = IdentitySub->GetLocalUserId().GetUniqueNetId();
	if (!LocalUserId.IsValid()) return;

	FOnlineUserPresenceStatus PresenceStatus;
	PresenceStatus.State = EOnlinePresenceState::Online;
	PresenceStatus.Properties.Add(TEXT("LobbyReadyState"), FVariantData(bLocalPlayerReady ? TEXT("1") : TEXT("0")));
	PresenceStatus.Properties.Add(TEXT("LobbyDisplayName"), FVariantData(IdentitySub->GetLocalDisplayName()));

	// Pass the new PresenceStatus object directly into SetPresence
	Presence->SetPresence(*LocalUserId, PresenceStatus,
		IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleSetPresenceComplete));
}

void UEOSLobbySubsystem::UpdateLobbyGlobalData()
{
	if (!IsLobbyLeader()) return;

	IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(LobbySessionName) : nullptr;
	if (!Session) return;

	Session->SessionSettings.Set(TEXT("MatchStatus"), CachedMatchStatus, EOnlineDataAdvertisementType::ViaOnlineService);
	if (!CachedServerIP.IsEmpty())
		Session->SessionSettings.Set(TEXT("LobbyServerIP"), CachedServerIP, EOnlineDataAdvertisementType::ViaOnlineService);

	Sessions->UpdateSession(LobbySessionName, Session->SessionSettings, true);
}
void UEOSLobbySubsystem::HandleSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& UpdatedSettings)
{
	// When the leader updates the match status or the server IP, EOS fires this on the clients
	if (SessionName == LobbySessionName)
	{
		RefreshMemberList();
	}
}
void UEOSLobbySubsystem::RefreshMemberList()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(LobbySessionName) : nullptr;
	if (!Session) return;

	IOnlineIdentityPtr  Identity = GetOSS() ? GetOSS()->GetIdentityInterface() : nullptr;
	IOnlinePresencePtr  PresenceInterface = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
	TSharedPtr<const FUniqueNetId> LocalId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;

	TArray<FLobbyMemberInfo> NewMembers;
	for (const FUniqueNetIdPtr& UserId : TrackedUserIds)
	{
		if (!UserId.IsValid()) continue;

		FLobbyMemberInfo Member;
		Member.UniqueIdString = UserId->ToString();
		const bool bIsLocalPlayer = LocalId.IsValid() && (*LocalId == *UserId);

		if (bIsLocalPlayer)
		{
			if (UEOSIdentitySubsystem* IDSub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>())
				Member.DisplayName = IDSub->GetLocalDisplayName();
		}
		else if (PresenceInterface.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> CachedPresence;
			if (PresenceInterface->GetCachedPresence(*UserId, CachedPresence) == EOnlineCachedResult::Success)
				if (const FVariantData* V = CachedPresence->Status.Properties.Find(TEXT("LobbyDisplayName")))
					V->GetValue(Member.DisplayName);
		}

		if (Member.DisplayName.IsEmpty())
			Member.DisplayName = FString::Printf(TEXT("Player_%s"), *Member.UniqueIdString.Right(4));

		if (bIsLocalPlayer)
		{
			Member.bIsReady = bLocalPlayerReady;
		}
		else if (PresenceInterface.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> CachedPresence;
			if (PresenceInterface->GetCachedPresence(*UserId, CachedPresence) == EOnlineCachedResult::Success)
			{
				if (const FVariantData* V = CachedPresence->Status.Properties.Find(TEXT("LobbyReadyState")))
				{
					FString ReadyString;
					V->GetValue(ReadyString);
					Member.bIsReady = (ReadyString == TEXT("1"));
				}
			}
		}

		NewMembers.Add(Member);
	}

	CurrentMembers = NewMembers;

	FString MatchStatus;
	FString FoundServerIP;
	Session->SessionSettings.Get(TEXT("MatchStatus"), MatchStatus);
	Session->SessionSettings.Get(TEXT("LobbyServerIP"), FoundServerIP);

	if (!MatchStatus.IsEmpty())
		OnMatchmakingStatusUpdated.Broadcast(MatchStatus);

	if (!FoundServerIP.IsEmpty() && !bHasStartedTeleport)
	{
		bHasStartedTeleport = true;
		OnMatchReadyToJoin.Broadcast(FoundServerIP);
	}

	if (CurrentMembers.Num() > 0)
	{
		OnLobbyMembersUpdated.Broadcast(CurrentMembers);

		// Trigger the 3D model spawn/update logic now that the array is accurate
		UpdateLobbyAvatars();
	}

	// If everyone in the current lobby clicks ready, the leader fires the HTTP request.
	if (IsLobbyLeader() && !bIsMatchmakingStarted && CurrentMembers.Num() > 0 && AreAllPlayersReady())
	{
		bIsMatchmakingStarted = true;
		if (UEOSMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UEOSMatchmakingSubsystem>())
		{
			MatchSub->StartMatchmaking();
		}
	}
}

void UEOSLobbySubsystem::ClearLobbyAvatars()
{
	// Loop backwards when removing/destroying things from an array
	for (int32 i = SpawnedAvatars.Num() - 1; i >= 0; --i)
	{
		if (SpawnedAvatars[i].IsValid())
		{
			SpawnedAvatars[i].Get()->Destroy();
		}
	}
	SpawnedAvatars.Empty();
}
void UEOSLobbySubsystem::UpdateLobbyAvatars()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Use a timer to delay visual spawning to the next frame.
	// This entirely disconnects the render thread allocation from the EOS callback stack, preventing D3D12 locks.
	World->GetTimerManager().SetTimerForNextTick([this, WeakThis = TWeakObjectPtr<UEOSLobbySubsystem>(this)]()
		{
			if (!WeakThis.IsValid()) return;

			UWorld* ValidWorld = WeakThis->GetWorld();

			// CRITICAL: Do not attempt to spawn if the world is invalid or currently tearing down
			if (!ValidWorld || ValidWorld->bIsTearingDown || !WeakThis->AvatarActorClass) return;

			WeakThis->ClearLobbyAvatars();

			// Fallback transforms if none are set in the Blueprint
			if (WeakThis->PlayerSpawnTransforms.IsEmpty())
			{
				WeakThis->PlayerSpawnTransforms.Add(FTransform(FRotator(0.f, 180.f, 0.f), FVector(100.f, -100.f, 0.f))); // Player 1
				WeakThis->PlayerSpawnTransforms.Add(FTransform(FRotator(0.f, 180.f, 0.f), FVector(100.f, 100.f, 0.f)));  // Player 2
			}

			const int32 MaxPlayers = 2;

			// Spawn an avatar for each member currently in the lobby
			for (int32 i = 0; i < WeakThis->CurrentMembers.Num() && i < MaxPlayers; ++i)
			{
				int32 TransformIndex = FMath::Min(i, WeakThis->PlayerSpawnTransforms.Num() - 1);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AActor* NewAvatar = ValidWorld->SpawnActor<AActor>(
					WeakThis->AvatarActorClass,
					WeakThis->PlayerSpawnTransforms[TransformIndex],
					SpawnParams
				);

				if (NewAvatar)
				{
					WeakThis->SpawnedAvatars.Add(NewAvatar);
				}
			}
		});
}
bool UEOSLobbySubsystem::AreAllPlayersReady() const
{
	if (CurrentMembers.IsEmpty()) return false;
	for (const FLobbyMemberInfo& M : CurrentMembers)
		if (!M.bIsReady) return false;
	return true;
}

IOnlineSubsystem* UEOSLobbySubsystem::GetOSS()              const { return IOnlineSubsystem::Get(); }
IOnlineSessionPtr  UEOSLobbySubsystem::GetSessionInterface()  const { return GetOSS() ? GetOSS()->GetSessionInterface() : nullptr; }