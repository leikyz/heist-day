#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "EOSMatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, FString, StatusMessage);

UCLASS()
class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// You will trigger your custom Go backend logic from here later
	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void StartMatchmaking();

	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
	FOnMatchmakingStatusChanged OnMatchmakingStatusChanged;

private :
	TSharedPtr<IWebSocket> MatchmakingSocket;

	void OnSocketConnected();
	void OnSocketMessage(const FString& Message);
	void OnSocketError(const FString& Error);
	void OnSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
};