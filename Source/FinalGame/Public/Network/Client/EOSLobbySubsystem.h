#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "FindSessionsCallbackProxy.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "EOSLobbySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLobbyMemberInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString UniqueIdString;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyJoined);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchReadyToJoin, const FString&, ConnectionString);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyMembersUpdated, const TArray<FLobbyMemberInfo>&, Members);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusUpdated, const FString&, StatusMessage);

UCLASS()
class FINALGAME_API UEOSLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Initialization

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Resets lobby state variables for a new match while keeping players grouped
	UFUNCTION(BlueprintCallable, Category = "EOS Lobby")
	void ResetLobbyForNextMatch();

#pragma endregion

#pragma region Lobby API

	// Creates a new Party Lobby via EOS Sessions
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void CreateOwnLobby();

	// Joins an existing lobby from a search result or invite
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void JoinLobby(const FBlueprintSessionResult& SearchResult);

	// Leaves and destroys the current lobby session
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void LeaveLobby();

	// Broadcasts the dedicated server IP to all clients in the lobby
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void BroadcastServerIP(const FString& ConnectionString);

	// Broadcasts server IP along with team assignment data
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void BroadcastMatchInfo(const FString& ConnectionString, const FString& TeamAssignmentsJson);

	// Syncs the current matchmaking state message to clients
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SyncMatchmakingState(const FString& StatusMessage);

	// Opens the Epic Social Overlay for invites and friends
	UFUNCTION(BlueprintCallable, Category = "EOS|Overlay")
	void ShowEpicOverlay();

#pragma endregion

#pragma region Player State

	// Updates the local player's readiness state and pushes it via Presence
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SetLocalPlayerReady(bool bReady);

	// Checks if all current lobby members have set their state to ready
	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool AreAllPlayersReady() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	FString GetLobbyId() const { return LobbyId; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLobbyLeader() const { return bIsOwner; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	TArray<FLobbyMemberInfo> GetCurrentMembers() const { return CurrentMembers; }

	UFUNCTION(BlueprintPure, Category = "EOS Lobby")
	bool GetIsInLobby() const { return bIsInLobby; }

#pragma endregion

#pragma region Events

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyCreated OnLobbyCreated;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyJoined OnLobbyJoined;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyLeft OnLobbyLeft;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyMembersUpdated OnLobbyMembersUpdated;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnMatchReadyToJoin OnMatchReadyToJoin;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnMatchmakingStatusUpdated OnMatchmakingStatusUpdated;

	UFUNCTION()
	void HandleMatchReadyToJoin(const FString& ConnectionString);

#pragma endregion

#pragma region Visuals

	// Class used to represent players in the 3D lobby background
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby Visuals")
	TSubclassOf<AActor> AvatarActorClass;

	// Configurable spawn locations for Player 1 (Index 0) and Player 2 (Index 1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby Visuals")
	TArray<FTransform> PlayerSpawnTransforms;

#pragma endregion

private:
#pragma region Session Callbacks

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId);
	void HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason Reason);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& UpdatedSettings);

#pragma endregion

#pragma region Presence Callbacks

	void HandlePresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence);
	void HandleSetPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful);
	void HandleQueryPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful);

#pragma endregion

#pragma region Internal Helpers

	void RefreshMemberList();
	void UpdatePresenceData();
	void UpdateLobbyGlobalData();
	void QueryPresenceForTrackedUsers();

	IOnlineSubsystem* GetOSS() const;
	IOnlineSessionPtr GetSessionInterface() const;

	void UpdateLobbyAvatars();
	void ClearLobbyAvatars();

#pragma endregion

#pragma region Cached Data

	TArray<FLobbyMemberInfo> CurrentMembers;
	TArray<FUniqueNetIdPtr> TrackedUserIds;

	bool bIsInLobby = false;
	bool bIsOwner = false;
	bool bLocalPlayerReady = false;
	bool bIsMatchmakingStarted = false;
	bool bHasStartedTeleport = false;
	bool bLeavingToJoin = false;

	FString LobbyId;
	FString CachedTeamAssignmentsJson;
	FString CachedServerIP;
	FString CachedMatchStatus = TEXT("Idle");
	FBlueprintSessionResult PendingJoinResult;

	FDelegateHandle OnSessionParticipantJoinedHandle;
	FDelegateHandle OnSessionParticipantLeftHandle;
	FDelegateHandle OnCreateSessionHandle;
	FDelegateHandle OnJoinSessionHandle;
	FDelegateHandle OnDestroySessionHandle;
	FDelegateHandle OnPresenceReceivedHandle;
	FDelegateHandle OnSessionUserInviteAcceptedHandle;
	FDelegateHandle OnSetPresenceCompleteHandle;
	FDelegateHandle OnSessionSettingsUpdatedHandle;

	TArray<TWeakObjectPtr<AActor>> SpawnedAvatars;

	static const FName LobbySessionName;

#pragma endregion
};