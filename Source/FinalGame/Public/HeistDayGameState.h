#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "HeistDayPlayerState.h"
#include "HeistDayGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    WaitingForPlayers,
    PreRound,
    FirstRound,
    FirstRoundEnd,
    SecondRound,
    SecondRoundEnd,
    MatchEnd
};

USTRUCT(BlueprintType)
struct FTeamData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TeamId = 0;

    UPROPERTY(BlueprintReadOnly)
    ETeam Team = ETeam::None;

    UPROPERTY(BlueprintReadOnly)
    TArray<AHeistDayPlayerState*> Players;

    UPROPERTY(BlueprintReadOnly)
    int32 Round1Score = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Round2Score = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemainingTimeChanged, float, RemainingSeconds);

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
    // Events
    UPROPERTY(BlueprintAssignable)
    FOnMatchPhaseChanged OnMatchPhaseChanged;

    UPROPERTY(BlueprintAssignable)
    FOnRemainingTimeChanged OnRemainingTimeChanged;

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