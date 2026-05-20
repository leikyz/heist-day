#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "EOSMatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchFound, const FString&, ConnectionString);

UCLASS()
class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

#pragma region Matchmaking API

	// Initiates a matchmaking request to the custom Go backend
	UFUNCTION(BlueprintCallable, Category = "Matchmaking")
	void StartMatchmaking();

#pragma endregion

#pragma region Events

	// Fired when the backend successfully returns a dedicated server IP
	UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
	FOnMatchFound OnMatchFound;

#pragma endregion

#pragma region Match Data

	// The team ID assigned to the local player by the Go backend
	UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
	int32 PendingTeamID = -1;

	// The IP address and port of the allocated dedicated server
	UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
	FString PendingServerIP;

#pragma endregion

private:

#pragma region HTTP Callbacks

	// Handles the response from the Go backend matchmaking ticket
	void OnMatchmakingResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

#pragma endregion
};