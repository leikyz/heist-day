//#pragma once
//
//#include "CoreMinimal.h"
//#include "Subsystems/GameInstanceSubsystem.h"
//#include "IWebSocket.h"
//#include "EOSMatchmakingSubsystem.generated.h"
//
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, FString, StatusMessage);
//
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartyReadyUpdate, int32, ReadyCount, int32, TotalPlayers);
//
//UCLASS()
//class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
//{
//	GENERATED_BODY()
//
//public:
//	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
//
//	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
//	void StartMatchmaking();
//
//	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
//	void ConnectToPartyTracker(FString LobbyIdString);
//
//
//	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
//	void SendReadyState(bool bIsReady);
//
//	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
//	void DisconnectTracker();
//
//	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
//	FOnMatchmakingStatusChanged OnMatchmakingStatusChanged;
//
//	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
//	FOnPartyReadyUpdate OnPartyReadyUpdate;
//
//private:
//	TSharedPtr<IWebSocket> PartySocket;
//	TSharedPtr<IWebSocket> MatchmakingSocket;
//
//	 Callbacks for Party Socket
//	void OnPartySocketConnected();
//	void OnPartySocketMessage(const FString& Message);
//	void OnPartySocketError(const FString& Error);
//	void OnPartySocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
//
//	 Callbacks for Direct Matchmaking Socket
//	void OnMatchmakingSocketConnected();
//	void OnMatchmakingSocketMessage(const FString& Message);
//	void OnMatchmakingSocketError(const FString& Error);
//	void OnMatchmakingSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
//
//	 Helper to process JSON logic (reusable)
//	void ProcessServerMessage(const FString& Message);
//};