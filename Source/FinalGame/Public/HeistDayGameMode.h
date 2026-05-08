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

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Round")
    float RoundDuration = 180.f;

    UPROPERTY(EditDefaultsOnly, Category = "Round")
    int32 ExpectedPlayerCount = 2;



private:
    void StartRound();
    void OnRoundTimerExpired();

    UPROPERTY()
    AHeistDayGameState* CachedGameState = nullptr;

    FTimerHandle RoundTimerHandle;
    int32 ConnectedCount = 0;

    TSet<APlayerStart*> UsedPlayerStarts;
};