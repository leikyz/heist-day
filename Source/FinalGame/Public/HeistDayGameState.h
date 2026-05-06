#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HeistDayGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    WaitingForPlayers  UMETA(DisplayName = "Waiting for players"),
    Round1             UMETA(DisplayName = "Round 1"),
    BetweenRounds      UMETA(DisplayName = "Between rounds"),
    Round2             UMETA(DisplayName = "Round 2"),
    PostMatch          UMETA(DisplayName = "Post match")
};

USTRUCT(BlueprintType)
struct FRoundResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    int32 ThiefScore = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    int32 EmployeeScore = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundTimeUpdated, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundEnded, const FRoundResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, const FRoundResult&, Round1Result, const FRoundResult&, Round2Result);

UCLASS()
class FINALGAME_API AHeistDayGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AHeistDayGameState();

    UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Time")
    float RemainingTime;

    UFUNCTION()
    void OnRep_RemainingTime();

    // Replicated readable state

    UFUNCTION(BlueprintPure, Category = "Match")
    EMatchPhase GetMatchPhase() const { return MatchPhase; }

    // Seconds remaining in the current timed phase (Round or BetweenRounds). 
    UFUNCTION(BlueprintPure, Category = "Match")
    float GetRemainingTime() const { return RemainingTime; }

    UFUNCTION(BlueprintPure, Category = "Match")
    FRoundResult GetRound1Result() const { return Round1Result; }

    UFUNCTION(BlueprintPure, Category = "Match")
    FRoundResult GetRound2Result() const { return Round2Result; }

    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnMatchPhaseChanged OnMatchPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnRoundTimeUpdated OnRoundTimeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnRoundEnded OnRoundEnded;

    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnMatchEnded OnMatchEnded;

    // Server-only setters (called by GameMode)

    void Server_SetMatchPhase(EMatchPhase NewPhase);
    void Server_SetRemainingTime(float Seconds);
    void Server_SetRound1Result(const FRoundResult& Result);
    void Server_SetRound2Result(const FRoundResult& Result);

    // Multicast RPCs (GameMode calls these to notify all clients) 

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_NotifyRoundStarted(int32 RoundNumber);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_NotifyRoundEnded(const FRoundResult& Result);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_NotifyMatchEnded(const FRoundResult& R1, const FRoundResult& R2);

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    EMatchPhase MatchPhase = EMatchPhase::WaitingForPlayers;

    UPROPERTY(Replicated)
    FRoundResult Round1Result;

    UPROPERTY(Replicated)
    FRoundResult Round2Result;

    UFUNCTION()
    void OnRep_MatchPhase();
};