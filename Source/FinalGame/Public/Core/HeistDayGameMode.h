#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/HeistDayGameState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Interfaces/IHttpRequest.h" 
#include "HeistDayGameMode.generated.h"

USTRUCT(BlueprintType)
struct FCarryableSpawnData
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AActor> CarryableClass;

	UPROPERTY()
	FTransform InitialTransform;
};

UCLASS()
class FINALGAME_API AHeistDayGameMode : public AGameMode
{
	GENERATED_BODY()

public:

#pragma region Lifecycle & Connection

	virtual void BeginPlay() override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	void OnClientReady();

#pragma endregion

#pragma region Backend Communication

	// Notifies the custom Go backend that the dedicated server has loaded the map and is ready for players
	void NotifyServerReady();
	void OnServerReadyResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// Signals the backend that the match concluded, triggering server shutdown
	void NotifyMatchEndAndShutdown();
	void OnMatchEndResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

#pragma endregion

#pragma region Spawning & Environment Reset

	virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
	TSubclassOf<AActor> CarryableBaseClass;

	UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
	TSubclassOf<AActor> KeycardBaseClass;

	UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
	TSubclassOf<AActor> FracturableBaseClass;

	// Caches the initial transforms of interactable actors to respawn them perfectly at halftime
	void SaveCarryablesInitialState();

	UFUNCTION(BlueprintCallable)
	void ResetCarryables();

#pragma endregion

#pragma region Combat & Stats Tracking

	UFUNCTION(BlueprintCallable)
	void HandlePlayerDamage(AController* Victim, AController* Attacker, float DamageAmount);

	UFUNCTION(BlueprintCallable)
	void HandleChangePlayerHealthValue(AController* Victim, int32 NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Match|Stats")
	void HandlePlayerInterception(AController* Interceptor);

	UFUNCTION(BlueprintCallable, Category = "Match|Stats")
	void HandlePlayerRobbed(AController* Robber);

#pragma endregion

#pragma region Scoring

	UFUNCTION(BlueprintCallable, Category = "Match|Score")
	void AwardThiefScore(int32 TeamId, int32 ScoreToAdd);

	UFUNCTION(BlueprintCallable, Category = "Match|Score")
	void AwardEmployeeScore(int32 TeamId, int32 ScoreToAdd);

#pragma endregion

#pragma region Round Management

	// Updates the round timer, usually triggered when an alarm goes off
	void SetAlarmTimer(float NewTime);

#pragma endregion

protected:

#pragma region Round Configuration

	UPROPERTY(EditDefaultsOnly, Category = "Round")
	float RoundDuration = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Round")
	int32 ExpectedPlayerCount = 4;

#pragma endregion

private:

#pragma region Internal Game Flow (Game loop)

	void StartRound(int roundNumber);
	void OnRoundTimerExpired();
	bool CheckAllThiefDead();
	void SwapAllTeamsRoles();
	void ResetAllPlayersHealth();
	void TeleportPlayersToNewSpawns();
	int32 GetNumValidPlayerStarts() const;
	void NotifyServerReadyWithRetry();

#pragma endregion

#pragma region Cached Data

	UPROPERTY()
	AHeistDayGameState* CachedGameState = nullptr;

	FTimerHandle RoundTimerHandle;

	int32 ConnectedCount = 0;
	int32 ReadyPlayersCount = 0;
	int32 ThiefCount = 0;
	int32 EmployeeCount = 0;

	TSet<APlayerStart*> UsedPlayerStarts;

	UPROPERTY()
	TArray<FCarryableSpawnData> InitialCarryablesData;

#pragma endregion
};