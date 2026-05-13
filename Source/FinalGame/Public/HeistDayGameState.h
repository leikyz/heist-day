#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "HeistDayPlayerState.h"
#include "HeistDayGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    WaitingForPlayers,
    FirstRoundStart,
    FirstRound,
    FirstRoundEnd,
    SecondRoundStart,
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
    int32 ThiefScore = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EmployeeScore = 10000;
};

USTRUCT(BlueprintType)
struct FMatchData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FTeamData FirstTeam;

    UPROPERTY(BlueprintReadOnly)
    FTeamData SecondTeam;

    FTeamData MatchWinnerTeam;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemainingTimeChanged, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamValueChanged, FTeamData, TeamData);

UCLASS()
class FINALGAME_API AHeistDayGameState : public AGameState
{
    GENERATED_BODY()

public:
    AHeistDayGameState();

    friend class AHeistDayGameMode;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void Tick(float DeltaSeconds) override;

    void Server_SetRemainingTime(float Seconds);
    void Server_SetMatchPhase(EMatchPhase NewPhase);

    void Server_SetThiefScore(int32 TeamId, int32 ScoreToAdd);
    void Server_SetEmployeeScore(int32 TeamId, int32 ScoreToAdd);

    UFUNCTION(BlueprintPure)
    float GetRemainingTime() const { return RemainingTime; }


    UFUNCTION(BlueprintPure, Category = "Match")
    FMatchData GetCurrentMatchData() const { return CurrentMatchData; }

    UFUNCTION(BlueprintPure)
    EMatchPhase GetMatchPhase() const { return MatchPhase; }

    // Events
    UPROPERTY(BlueprintAssignable)
    FOnMatchPhaseChanged OnMatchPhaseChanged;

    UPROPERTY(BlueprintAssignable)
    FOnTeamValueChanged OnTeamValueChanged;

    UPROPERTY(BlueprintAssignable)
    FOnRemainingTimeChanged OnRemainingTimeChanged;

    UFUNCTION(BlueprintPure, Category = "Match")
    FTeamData GetTeamDataById(int32 TeamId) const;

    UFUNCTION(BlueprintPure, Category = "Match")
    FTeamData GetOpposingTeamDataById(int32 TeamId) const;


    UFUNCTION(BlueprintCallable, Category = "Match")
    bool GetMatchWinner(FTeamData& OutWinner);


    UFUNCTION(BlueprintCallable, Category = "Match")
    bool GetMatchLooser(FTeamData& OutLooser);

private:
    UPROPERTY(ReplicatedUsing = OnRep_RemainingTime)
    float RemainingTime = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    EMatchPhase MatchPhase = EMatchPhase::WaitingForPlayers;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentMatchData)
    FMatchData CurrentMatchData;

    UFUNCTION()
    void OnRep_RemainingTime();

    UFUNCTION()
    void OnRep_MatchPhase();

    UFUNCTION()
    void OnRep_CurrentMatchData();
};