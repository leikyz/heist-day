#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EOSLobbySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyFinished, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerReadyUpdate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyMatchmakingStateSync, FString, State);

USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "EOS")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "EOS")
	bool bIsReady;

	UPROPERTY(BlueprintReadOnly, Category = "EOS")
	bool bIsLobbyLeader;
};

UCLASS()
class FINALGAME_API UEOSLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void CreateLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void FindAndJoinLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void LeaveLobby();

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsInLobby() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	int32 GetLobbyPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void NotifyLobbyStateChanged();

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	TArray<FLobbyPlayerInfo> GetLobbyPlayersInfo();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void StartGame(FString ServerIP);

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SyncMatchmakingState(FString NewState);

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsLobbyLeader() const;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void SetPlayerReady(bool bIsReady);

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyStateChanged OnLobbyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLeaveLobbyFinished OnLeaveLobbyFinished;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyMatchmakingStateSync OnLobbyMatchmakingStateSync;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnPlayerReadyUpdate OnPlayerReadyUpdate;

private:
	IOnlineSessionPtr GetSessionInterface() const;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnSessionSettingsUpdated(FName SessionName, const FOnlineSessionSettings& UpdatedSettings);

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
	bool bIsReadyLocal = false; // Local tracking for the stub
};