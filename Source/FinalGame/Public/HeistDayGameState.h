#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "HeistDayGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    WaitingForPlayers,
    FirstRound,
    FirstRoundEnd,
    SecondRound,
    SecondRoundEnd,
    MatchEnd
};

UCLASS()
class FINALGAME_API AHeistDayGameState : public AGameState
{
    GENERATED_BODY()

public:
    AHeistDayGameState();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void Tick(float DeltaSeconds) override;

    void Server_SetRemainingTime(float Seconds);
    void Server_SetMatchPhase(EMatchPhase NewPhase);

    UFUNCTION(BlueprintPure)
    float GetRemainingTime() const { return RemainingTime; }

    UFUNCTION(BlueprintPure)
    EMatchPhase GetMatchPhase() const { return MatchPhase; }

private:
    UPROPERTY(ReplicatedUsing = OnRep_RemainingTime)
    float RemainingTime = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    EMatchPhase MatchPhase = EMatchPhase::WaitingForPlayers;

    UFUNCTION()
    void OnRep_RemainingTime();

    UFUNCTION()
    void OnRep_MatchPhase();
};