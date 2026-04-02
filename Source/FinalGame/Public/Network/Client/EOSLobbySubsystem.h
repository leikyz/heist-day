#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Network/EOSNetworkSubsystem.h"
#include "EOSLobbySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyFinished, bool, bWasSuccessful);
UCLASS()
class FINALGAME_API UEOSLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void CreateLobby();

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void LeaveLobby();

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	bool IsInLobby() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	int32 GetLobbyPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void NotifyLobbyStateChanged();

	UFUNCTION(BlueprintPure, Category = "EOS|Lobby")
	TArray<FString> GetLobbyMemberNames();

	// The Leader calls this to pull the whole lobby into the game
	UFUNCTION(BlueprintCallable, Category = "EOS|Lobby")
	void StartGame(FString ServerIP);

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLobbyStateChanged OnLobbyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
	FOnLeaveLobbyFinished OnLeaveLobbyFinished;

private:
	void OnCreateLobbyComplete(const UE::Online::TOnlineResult<UE::Online::FCreateLobby>& Result);
	void OnLeaveLobbyComplete(const UE::Online::TOnlineResult<UE::Online::FLeaveLobby>& Result);

	UE::Online::FOnlineEventDelegateHandle UIJoinRequestedHandle;

	void OnLobbyJoinRequested(const UE::Online::FUILobbyJoinRequested& Event);
	void OnJoinLobbyComplete(const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& Result);

	void OnLobbyMemberJoined(const UE::Online::FLobbyMemberJoined& Event);
	void OnLobbyMemberLeft(const UE::Online::FLobbyMemberLeft& Event);

	// The new clean function to handle when the Leader broadcasts the Server IP
	void OnLobbyAttributesUpdate(const UE::Online::FLobbyAttributesChanged& Event);



	TSharedPtr<const UE::Online::FLobby> CachedLobbyObject;

	// Pending Join Logic
	void ProcessJoinLobby(UE::Online::FAccountId InLocalAccountId, UE::Online::FLobbyId InLobbyId);
	UE::Online::FLobbyId PendingLobbyId;
	UE::Online::FAccountId PendingAccountId;
	bool bHasPendingJoin = false;

	UE::Online::FLobbyId CurrentLobbyId;
	UE::Online::FOnlineEventDelegateHandle MemberJoinedHandle;
	UE::Online::FOnlineEventDelegateHandle MemberLeftHandle;
	UE::Online::FOnlineEventDelegateHandle AttributesChangedHandle;

	int32 CurrentPlayerCount = 0;
};