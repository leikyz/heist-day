#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Online/OnlineServices.h"
#include "Online/Sessions.h"
#include "EOSMatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, FString, StatusMessage);

UCLASS()
class FINALGAME_API UEOSMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Matchmaking")
	void StartMatchmaking();

	UPROPERTY(BlueprintAssignable, Category = "EOS|Matchmaking")
	FOnMatchmakingStatusChanged OnMatchmakingStatusChanged;

private:
	void FindAvailableMatch();
	void OnMatchJoined(const UE::Online::TOnlineResult<UE::Online::FJoinSession>& Result);

	// Polling function called by the timer
	void CheckMatchStatus();

	FTimerHandle MatchTimerHandle;
	UE::Online::IOnlineServicesPtr GetServices() const;
};