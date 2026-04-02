#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Engine/TimerHandle.h" // Needed for FTimerHandle
#include "EOSMatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, FString, StatusMessage);

UCLASS()
class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Starts the matchmaking process
	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void FindMatch(int32 PartySize);

	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
	FOnMatchmakingStatusChanged OnStatusChanged;

private:
	void SearchForSession(int32 SlotsNeeded);
	void CreateMatchmakingSession(int32 SlotsNeeded);
	void JoinFoundSession(const FOnlineSessionSearchResult& SearchResult);

	// OSSv1 Callbacks
	void OnSearchCompleted(bool bWasSuccessful);
	void OnCreateCompleted(FName SessionName, bool bWasSuccessful);
	void OnJoinCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// NEW: Polling system to check the Epic Bulletin Board
	void PollSessionSize();
	FTimerHandle PollingTimer;

	int32 CurrentPartySize = 1;

	// Holds the search results in memory
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
};