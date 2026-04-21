#include "Network/Client/EOSLobbySubsystem.h"
#include "Network/Client/EOSIdentitySubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineUserInterface.h"

const FName UEOSLobbySubsystem::LobbySessionName = FName(TEXT("EOSLobby"));
static const FString ReadyKeyPrefix = TEXT("READY_");

void UEOSLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions) return;

	OnSessionUserInviteAcceptedHandle = Sessions->OnSessionUserInviteAcceptedDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionUserInviteAccepted);
	OnSessionParticipantJoinedHandle = Sessions->OnSessionParticipantJoinedDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionParticipantJoined);
	OnSessionParticipantLeftHandle = Sessions->OnSessionParticipantLeftDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionParticipantLeft);
	OnSessionSettingsUpdatedHandle = Sessions->OnSessionSettingsUpdatedDelegates.AddUObject(this, &UEOSLobbySubsystem::HandleSessionSettingsUpdated);

	IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
	if (Presence.IsValid())
	{
		OnPresenceReceivedHandle = Presence->AddOnPresenceReceivedDelegate_Handle(FOnPresenceReceivedDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandlePresenceReceived));
	}

	UE_LOG(LogTemp, Log, TEXT("[EOSLobby] Subsystem initialized."));
}

void UEOSLobbySubsystem::Deinitialize()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions)
	{
		Sessions->OnSessionUserInviteAcceptedDelegates.Remove(OnSessionUserInviteAcceptedHandle);
		Sessions->OnSessionParticipantJoinedDelegates.Remove(OnSessionParticipantJoinedHandle);
		Sessions->OnSessionParticipantLeftDelegates.Remove(OnSessionParticipantLeftHandle);
		Sessions->OnSessionSettingsUpdatedDelegates.Remove(OnSessionSettingsUpdatedHandle);

		IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
		if (Presence.IsValid())
		{
			Presence->ClearOnPresenceReceivedDelegate_Handle(OnPresenceReceivedHandle);
		}
	}
	Super::Deinitialize();
}

void UEOSLobbySubsystem::HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId)
{
	if (SessionName != LobbySessionName) return;

	UE_LOG(LogTemp, Log, TEXT("[EOSLobby] Remote Join Detected. Querying name for: %s"), *UniqueId.ToString());
	TrackedUserIds.AddUnique(UniqueId.AsShared());

	IOnlineUserPtr UserInterface = GetUserInterface();
	if (UserInterface.IsValid())
	{
		TArray<FUniqueNetIdRef> UsersToQuery;
		UsersToQuery.Add(UniqueId.AsShared());

		UserInterface->OnQueryUserInfoCompleteDelegates[0].AddUObject(this, &UEOSLobbySubsystem::HandleQueryUserInfoComplete);
		UserInterface->QueryUserInfo(0, UsersToQuery);
	}
	else
	{
		RefreshMemberList();
	}
}

void UEOSLobbySubsystem::HandleQueryUserInfoComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FUniqueNetIdRef>& UserIds, const FString& ErrorStr)
{
	if (IOnlineUserPtr UserInterface = GetUserInterface())
	{
		UserInterface->OnQueryUserInfoCompleteDelegates[LocalUserNum].RemoveAll(this);
	}
	RefreshMemberList();
}

void UEOSLobbySubsystem::RefreshMemberList()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* Session = Sessions ? Sessions->GetNamedSession(LobbySessionName) : nullptr;
	if (!Session) return;

	TArray<FLobbyMemberInfo> NewMembers;
	IOnlineIdentityPtr Identity = GetOSS() ? GetOSS()->GetIdentityInterface() : nullptr;
	IOnlineUserPtr UserInterface = GetUserInterface();
	IOnlinePresencePtr PresenceInterface = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;

	for (const FUniqueNetIdPtr& UserId : TrackedUserIds)
	{
		if (!UserId.IsValid()) continue;

		FLobbyMemberInfo Member;
		Member.UniqueIdString = UserId->ToString();
		Member.DisplayName = TEXT("");

		// Check Identity Interface (Primarily finds the Local Player)
		if (Identity.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> Account = Identity->GetUserAccount(*UserId);
			if (Account.IsValid())
			{
				Member.DisplayName = Account->GetDisplayName();
			}
		}

		// Check User Interface (Where QueryUserInfo results are stored for Remote Players)
		if (Member.DisplayName.IsEmpty() && UserInterface.IsValid())
		{
			TSharedPtr<FOnlineUser> User = UserInterface->GetUserInfo(0, *UserId);
			if (User.IsValid())
			{
				Member.DisplayName = User->GetDisplayName();
			}
		}

		// Fallback: Identity Nickname
		if (Member.DisplayName.IsEmpty() && Identity.IsValid())
		{
			Member.DisplayName = Identity->GetPlayerNickname(*UserId);
		}

		// Final Fail-Safe: If still empty, use the truncated ID
		if (Member.DisplayName.IsEmpty())
		{
			Member.DisplayName = FString::Printf(TEXT("Player_%s"), *Member.UniqueIdString.Right(6));
		}

		// Fetch Ready State from Presence (Bypasses Host-Only Restrictions)
		Member.bIsReady = false;

		// Check if we are evaluating the local player (instant UI update)
		if (Identity.IsValid() && Identity->GetUniquePlayerId(0).IsValid() && *Identity->GetUniquePlayerId(0) == *UserId)
		{
			Member.bIsReady = bLocalPlayerReady;
		}
		// Otherwise, read the remote player's cached Presence data
		else if (PresenceInterface.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> CachedPresence;
			if (PresenceInterface->GetCachedPresence(*UserId, CachedPresence) == EOnlineCachedResult::Success)
			{
				if (const FVariantData* ReadyData = CachedPresence->Status.Properties.Find(TEXT("LobbyReadyState")))
				{
					FString ReadyString;
					ReadyData->GetValue(ReadyString);
					Member.bIsReady = (ReadyString == TEXT("1"));
				}
			}
		}

		NewMembers.Add(Member);
	}

	CurrentMembers = NewMembers;
	OnLobbyMembersUpdated.Broadcast(CurrentMembers);
}

void UEOSLobbySubsystem::CreateOwnLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions) return;
	if (Sessions->GetNamedSession(LobbySessionName) != nullptr) { Sessions->DestroySession(LobbySessionName); return; }

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false; Settings.bIsDedicated = false; Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true; Settings.bAllowInvites = true; Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true; Settings.NumPublicConnections = 4; Settings.BuildUniqueId = 1;

	UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	FString LocalName = IdentitySub ? IdentitySub->GetLocalDisplayName() : TEXT("Unknown");
	Settings.Set(FName(TEXT("OWNER_NAME")), LocalName, EOnlineDataAdvertisementType::ViaOnlineService);

	OnCreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleCreateSessionComplete));
	Sessions->CreateSession(0, LobbySessionName, Settings);
}

void UEOSLobbySubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions) Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionHandle);
	if (bWasSuccessful)
	{
		bIsInLobby = true; bIsOwner = true;
		UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
		if (IdentitySub) TrackedUserIds.AddUnique(IdentitySub->GetLocalUserId().GetUniqueNetId());
		RefreshMemberList();
		OnLobbyCreated.Broadcast();
	}
}

void UEOSLobbySubsystem::JoinLobby(const FBlueprintSessionResult& SearchResult)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions) return;
	if (Sessions->GetNamedSession(LobbySessionName) != nullptr) { bLeavingToJoin = true; PendingJoinResult = SearchResult; LeaveLobby(); return; }

	OnJoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleJoinSessionComplete));
	UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	TSharedPtr<const FUniqueNetId> UserId = IdentitySub ? IdentitySub->GetLocalUserId().GetUniqueNetId() : nullptr;

	if (UserId.IsValid())
	{
		TrackedUserIds.AddUnique(SearchResult.OnlineResult.Session.OwningUserId);
		TrackedUserIds.AddUnique(UserId);
		Sessions->JoinSession(*UserId, LobbySessionName, SearchResult.OnlineResult);
	}
}

void UEOSLobbySubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionHandle);
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		bIsInLobby = true; bIsOwner = false;
		RefreshMemberList();
		OnLobbyJoined.Broadcast();
	}
}

void UEOSLobbySubsystem::LeaveLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions && Sessions->GetNamedSession(LobbySessionName))
	{
		OnDestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleDestroySessionComplete));
		Sessions->DestroySession(LobbySessionName);
	}
}

void UEOSLobbySubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions) Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionHandle);
	bIsInLobby = false; bIsOwner = false; CurrentMembers.Empty(); TrackedUserIds.Empty(); bLocalPlayerReady = false;
	if (bLeavingToJoin) { bLeavingToJoin = false; JoinLobby(PendingJoinResult); }
	else { OnLobbyLeft.Broadcast(); }
}

void UEOSLobbySubsystem::HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason Reason)
{
	if (SessionName == LobbySessionName)
	{
		TrackedUserIds.RemoveAll([&UniqueId](const FUniqueNetIdPtr& Element) { return Element.IsValid() && *Element == UniqueId; });
		RefreshMemberList();
	}
}

void UEOSLobbySubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid()) return;
	FBlueprintSessionResult BPSearchResult; BPSearchResult.OnlineResult = InviteResult;
	JoinLobby(BPSearchResult);
}

void UEOSLobbySubsystem::SetLocalPlayerReady(bool bReady)
{
	bLocalPlayerReady = bReady;

	IOnlinePresencePtr Presence = GetOSS() ? GetOSS()->GetPresenceInterface() : nullptr;
	UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();

	if (Presence.IsValid() && IdentitySub)
	{
		TSharedPtr<const FUniqueNetId> LocalUserId = IdentitySub->GetLocalUserId().GetUniqueNetId();
		if (LocalUserId.IsValid())
		{
			FOnlinePresenceSetPresenceParameters Params;

			// Create the data payload
			FVariantData ReadyData;
			ReadyData.SetValue(bReady ? TEXT("1") : TEXT("0"));

			// Create a temporary properties map and add the data to it
			FPresenceProperties NewProperties;
			NewProperties.Add(TEXT("LobbyReadyState"), ReadyData);

			// Assign the map to the TOptional property
			Params.Properties = NewProperties;

			Presence->SetPresence(*LocalUserId, MoveTemp(Params));
		}
	}

	// Instantly update our local UI so we don't have to wait for the server round-trip
	RefreshMemberList();
}
void UEOSLobbySubsystem::HandlePresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	// Only refresh if the user updating their presence is actually in our lobby
	for (const FUniqueNetIdPtr& TrackedId : TrackedUserIds)
	{
		if (TrackedId.IsValid() && *TrackedId == UserId)
		{
			RefreshMemberList();
			break;
		}
	}
}
void UEOSLobbySubsystem::UpdateSessionSettings()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* Session = Sessions ? Sessions->GetNamedSession(LobbySessionName) : nullptr;
	if (!Session) return;
	UEOSIdentitySubsystem* IdentitySub = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	if (!IdentitySub) return;
	FString LocalIdStr = IdentitySub->GetLocalUserId().GetUniqueNetId().IsValid() ? IdentitySub->GetLocalUserId().GetUniqueNetId()->ToString() : TEXT("LOCAL");
	FName ReadyKey = FName(*GetReadyKeyForPlayer(LocalIdStr));
	FString ReadyValue = bLocalPlayerReady ? FString(TEXT("1")) : FString(TEXT("0"));
	Session->SessionSettings.Set(ReadyKey, ReadyValue, EOnlineDataAdvertisementType::ViaOnlineService);
	Sessions->UpdateSession(LobbySessionName, Session->SessionSettings, true);
	RefreshMemberList();
}

void UEOSLobbySubsystem::SyncMatchmakingState(const FString& StatusMessage)
{
	if (!IsLobbyOwner()) return;
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* Session = Sessions ? Sessions->GetNamedSession(LobbySessionName) : nullptr;
	if (Session)
	{
		Session->SessionSettings.Set(FName(TEXT("MATCH_STATUS")), StatusMessage, EOnlineDataAdvertisementType::ViaOnlineService);
		Sessions->UpdateSession(LobbySessionName, Session->SessionSettings, true);
	}
}

void UEOSLobbySubsystem::StartGame()
{
	if (!IsLobbyOwner()) return;
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions)
	{
		OnStartSessionHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &UEOSLobbySubsystem::HandleStartSessionComplete));
		Sessions->StartSession(LobbySessionName);
	}
}

void UEOSLobbySubsystem::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions) Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionHandle);
}

void UEOSLobbySubsystem::HandleSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& Settings) { if (SessionName == LobbySessionName) RefreshMemberList(); }
bool UEOSLobbySubsystem::AreAllPlayersReady() const { if (CurrentMembers.Num() < 1) return false; for (const FLobbyMemberInfo& Member : CurrentMembers) { if (!Member.bIsReady) return false; } return true; }
FString UEOSLobbySubsystem::GetReadyKeyForPlayer(const FString& PlayerIdStr) const { return ReadyKeyPrefix + PlayerIdStr.Right(32); }

IOnlineSubsystem* UEOSLobbySubsystem::GetOSS() const { return IOnlineSubsystem::Get(); }
IOnlineSessionPtr UEOSLobbySubsystem::GetSessionInterface() const { return GetOSS() ? GetOSS()->GetSessionInterface() : nullptr; }
IOnlineUserPtr UEOSLobbySubsystem::GetUserInterface() const { return GetOSS() ? GetOSS()->GetUserInterface() : nullptr; }