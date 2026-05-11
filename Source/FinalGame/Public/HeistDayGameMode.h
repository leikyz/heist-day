#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "HeistDayGameState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "HeistDayGameMode.generated.h"

UCLASS()
class FINALGAME_API  AHeistDayGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }

    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void HandlePlayerDamage(AController* Victim, AController* Attacker, float DamageAmount);

    UFUNCTION(BlueprintCallable)
	void HandleChangePlayerHealthValue(AController* Victim, int32 NewHealth); 

    void OnClientReady();


    //void HandlePlayerDeath(AController* Victim);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Round")
    float RoundDuration = 20.f;

    UPROPERTY(EditDefaultsOnly, Category = "Round")
    int32 ExpectedPlayerCount = 2;
    int32 ReadyPlayersCount = 0;


private:
    void StartRound(int roundNumber);
    void OnRoundTimerExpired();

	bool CheckAllThiefDead();

    UPROPERTY()
    AHeistDayGameState* CachedGameState = nullptr;

    FTimerHandle RoundTimerHandle;
    int32 ConnectedCount = 0;

    int32 ThiefCount = 0;
    int32 EmployeeCount = 0;

    TSet<APlayerStart*> UsedPlayerStarts;
};