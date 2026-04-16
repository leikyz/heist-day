#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "EOSMatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, FString, StatusMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartyReadyUpdate, int32, ReadyCount, int32, TotalPlayers);

UCLASS()
class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void StartMatchmaking();

	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void ConnectToPartyTracker(FString LobbyIdString);


	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void SendReadyState(bool bIsReady);

	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void DisconnectTracker();

	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
	FOnMatchmakingStatusChanged OnMatchmakingStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
	FOnPartyReadyUpdate OnPartyReadyUpdate;

private:
	TSharedPtr<IWebSocket> MatchmakingSocket;

	void OnSocketConnected();
	void OnSocketMessage(const FString& Message);
	void OnSocketError(const FString& Error);
	void OnSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
};