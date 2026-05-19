#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "HeistDayGameState.h"
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
class FINALGAME_API  AHeistDayGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }

    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void BeginPlay() override;

    void NotifyServerReady();
    void OnServerReadyResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void NotifyMatchEndAndShutdown();
    void OnMatchEndResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
    TSubclassOf<AActor> CarryableBaseClass;

    UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
    TSubclassOf<AActor> KeycardBaseClass;

    UPROPERTY(EditDefaultsOnly, Category = "Round Reset")
    TSubclassOf<AActor> FracturableBaseClass;

    void SaveCarryablesInitialState();

    UFUNCTION(BlueprintCallable)
    void ResetCarryables();

    UFUNCTION(BlueprintCallable)
    void HandlePlayerDamage(AController* Victim, AController* Attacker, float DamageAmount);

    UFUNCTION(BlueprintCallable)
	void HandleChangePlayerHealthValue(AController* Victim, int32 NewHealth); 

    UFUNCTION(BlueprintCallable, Category = "Match|Score")
    void AwardThiefScore(int32 TeamId, int32 ScoreToAdd);

    UFUNCTION(BlueprintCallable, Category = "Match|Score")
    void AwardEmployeeScore(int32 TeamId, int32 ScoreToAdd);

    void OnClientReady();

    void SetAlarmTimer(float NewTime);



    //void HandlePlayerDeath(AController* Victim);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Round")
    float RoundDuration = 60.f;

    UPROPERTY(EditDefaultsOnly, Category = "Round")
    int32 ExpectedPlayerCount = 2;
    int32 ReadyPlayersCount = 0;


private:
    void StartRound(int roundNumber);
    void OnRoundTimerExpired();

	bool CheckAllThiefDead();

    void SwapAllTeamsRoles();

    void ResetAllPlayersHealth();

    void TeleportPlayersToNewSpawns();

    UPROPERTY()
    AHeistDayGameState* CachedGameState = nullptr;

    FTimerHandle RoundTimerHandle;
    int32 ConnectedCount = 0;

    int32 ThiefCount = 0;
    int32 EmployeeCount = 0;

    TSet<APlayerStart*> UsedPlayerStarts;

    UPROPERTY()
    TArray<FCarryableSpawnData> InitialCarryablesData;
};