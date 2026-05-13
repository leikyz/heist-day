#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HeistDayPlayerState.generated.h"
UENUM(BlueprintType)
enum class ETeam : uint8
{
    None     UMETA(DisplayName = "None"),
    Thief    UMETA(DisplayName = "Thief"),
    Employee UMETA(DisplayName = "Employee")
};

USTRUCT(BlueprintType)
struct FRoundData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 KillsCount = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 DeathsCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 InterceptionsCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 robbedCount = 0;

};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsChanged, AHeistDayPlayerState*, PlayerState);

UCLASS()
class FINALGAME_API AHeistDayPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AHeistDayPlayerState();

    static constexpr uint8 MaxHealth = 100;
    UFUNCTION(BlueprintPure, Category = "Player")
    FString GetEpicAccountName() const { return EpicAccountName; }

    UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetDeathsCount() const { return FirstRound.DeathsCount + SecondRound.DeathsCount; }

    UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetKillsCount() const { return FirstRound.KillsCount + SecondRound.KillsCount; }

    UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetInterceptionsCount() const { return FirstRound.InterceptionsCount + SecondRound.InterceptionsCount; }

    UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetRobbedCount() const { return FirstRound.robbedCount + SecondRound.robbedCount; }

    UFUNCTION(BlueprintPure)
    int32 GetTeamId() const { return TeamId; }

    UFUNCTION(BlueprintPure)
    ETeam GetTeam() const { return Team; }

    UFUNCTION(BlueprintPure)
	FRoundData GetFirstRoundData() const { return FirstRound; }

    UFUNCTION(BlueprintPure)
	FRoundData GetSecondRoundData() const { return SecondRound; }

	UFUNCTION(BlueprintPure)
	int32 GetPlayerIndex() const { return PlayerIndex; }


    UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, BlueprintReadOnly, Category = "Stats") 
        FRoundData FirstRound;

    UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, BlueprintReadOnly, Category = "Stats") 
        FRoundData SecondRound;

	UFUNCTION(BlueprintPure)
	int32 GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure)
	bool IsDead() const { return CurrentHealth <= 0; }

    UPROPERTY(BlueprintAssignable)
    FOnPlayerDied OnPlayerDied;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStatsChanged OnStatsChanged;

    // Server setters, clients receive the updated value in OnRep
    void SetTeamId(int32 NewTeamId);
    void SetTeam(ETeam NewTeam);
    void SetPlayerIndex(int32 PlayerIndex);
    void SetCurrentHealth(int32 NewHealth);
    void SetEpicAccountName(const FString& InName) { EpicAccountName = InName; }
    void AddKill(int32 RoundNumber);
    void AddDeath(int32 RoundNumber);
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    FString EpicAccountName;

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Match")
    void Server_ClientIsReady();

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
    void Server_SetEpicName(const FString& NewName);
private:
    UPROPERTY(ReplicatedUsing = OnRep_TeamId)
    int32 TeamId = 0;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
    int32 CurrentHealth = MaxHealth;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerIndex)
    int32 PlayerIndex = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Team)
    ETeam Team = ETeam::None;


    UFUNCTION()
    void OnRep_TeamId();

    UFUNCTION()
    void OnRep_CurrentHealth();

    UFUNCTION()
    void OnRep_StatsChanged();

    UFUNCTION()
    void OnRep_PlayerIndex();

    UFUNCTION()
    void OnRep_Team();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};