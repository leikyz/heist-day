#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineUserInterface.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyMembersUpdated, const TArray<FLobbyMemberInfo>&, Members);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberReadyChanged, const FString&, MemberName, bool, bIsReady);

UCLASS()
class FINALGAME_API UEOSLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Call this from Blueprint to manually create a lobby
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void CreateOwnLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void JoinLobby(const FBlueprintSessionResult& SearchResult);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void LeaveLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SetLocalPlayerReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SyncMatchmakingState(const FString& StatusMessage);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void StartGame();

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLocalPlayerReady() const { return bLocalPlayerReady; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool AreAllPlayersReady() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	TArray<FLobbyMemberInfo> GetCurrentMembers() const { return CurrentMembers; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsInLobby() const { return bIsInLobby; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLobbyOwner() const { return bIsOwner; }

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLobbyLeader() const { return bIsOwner; }

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyCreated OnLobbyCreated;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyJoined OnLobbyJoined;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyLeft OnLobbyLeft;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyMembersUpdated OnLobbyMembersUpdated;

private:
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId);
	void HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason Reason);
	void HandleSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& Settings);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleQueryUserInfoComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FUniqueNetIdRef>& UserIds, const FString& ErrorStr);
	void HandlePresenceReceived(const class FUniqueNetId& UserId, const TSharedRef<class FOnlineUserPresence>& Presence);

	void RefreshMemberList();
	void UpdateSessionSettings();
	FString GetReadyKeyForPlayer(const FString& PlayerIdStr) const;

	IOnlineSubsystem* GetOSS() const;
	IOnlineSessionPtr GetSessionInterface() const;
	IOnlineUserPtr GetUserInterface() const;

	TArray<FLobbyMemberInfo> CurrentMembers;
	TArray<FUniqueNetIdPtr> TrackedUserIds;

	bool bIsInLobby = false;
	bool bIsOwner = false;
	bool bLocalPlayerReady = false;
	bool bLeavingToJoin = false;
	FBlueprintSessionResult PendingJoinResult;

	FDelegateHandle OnSessionParticipantJoinedHandle;
	FDelegateHandle OnSessionParticipantLeftHandle;
	FDelegateHandle OnSessionSettingsUpdatedHandle;
	FDelegateHandle OnSessionUserInviteAcceptedHandle;
	FDelegateHandle OnCreateSessionHandle;
	FDelegateHandle OnJoinSessionHandle;
	FDelegateHandle OnDestroySessionHandle;
	FDelegateHandle OnStartSessionHandle;

	FDelegateHandle OnPresenceReceivedHandle;

	static const FName LobbySessionName;
};