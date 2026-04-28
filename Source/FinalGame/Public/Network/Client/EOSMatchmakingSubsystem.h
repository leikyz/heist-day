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
	UFUNCTION(BlueprintCallable, Category = "Matchmaking")
	void StartMatchmaking();

	UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
	FOnMatchFound OnMatchFound;

	UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
	int32 PendingTeamID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
	FString PendingServerIP;

private:
	void OnMatchmakingResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};