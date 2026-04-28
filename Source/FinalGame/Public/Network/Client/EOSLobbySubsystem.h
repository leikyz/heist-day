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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void CreateOwnLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void JoinLobby(const FBlueprintSessionResult& SearchResult);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void LeaveLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SetLocalPlayerReady(bool bReady);

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool AreAllPlayersReady() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLobbyLeader() const { return bIsOwner; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	TArray<FLobbyMemberInfo> GetCurrentMembers() const { return CurrentMembers; }

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void BroadcastServerIP(const FString& ConnectionString);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SyncMatchmakingState(const FString& StatusMessage);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby Visuals")
	TSubclassOf<AActor> AvatarActorClass;

	// Configurable spawn locations for Player 1 (Index 0) and Player 2 (Index 1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby Visuals")
	TArray<FTransform> PlayerSpawnTransforms;

private:
	// Session callbacks
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId);
	void HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason Reason);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& UpdatedSettings);
	// Presence callbacks
	void HandlePresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence);
	void HandleSetPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful);

	// Internal helpers
	void RefreshMemberList();
	void UpdatePresenceData();
	void UpdateLobbyGlobalData();
	// Queries presence for all peers so their display names are cached on the joining client
	void QueryPresenceForTrackedUsers();
	void HandleQueryPresenceComplete(const FUniqueNetId& UserId, const bool bWasSuccessful);

	IOnlineSubsystem* GetOSS() const;
	IOnlineSessionPtr GetSessionInterface() const;

	TArray<FLobbyMemberInfo> CurrentMembers;
	TArray<FUniqueNetIdPtr> TrackedUserIds;

	bool bIsInLobby = false;
	bool bIsOwner = false;
	bool bLocalPlayerReady = false;
	bool bIsMatchmakingStarted = false;
	bool bHasStartedTeleport = false;
	bool bLeavingToJoin = false;

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

	void UpdateLobbyAvatars();
	void ClearLobbyAvatars();

	static const FName LobbySessionName;
};