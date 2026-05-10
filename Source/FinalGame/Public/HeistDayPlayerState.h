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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);


UCLASS()
class FINALGAME_API AHeistDayPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    static constexpr uint8 MaxHealth = 100;

    UFUNCTION(BlueprintPure, Category = "Player")
    FString GetEpicAccountName() const { return EpicAccountName; }

    UFUNCTION(BlueprintPure)
    int32 GetTeamId() const { return TeamId; }

    UFUNCTION(BlueprintPure)
    ETeam GetTeam() const { return Team; }

	UFUNCTION(BlueprintPure)
	int32 GetPlayerIndex() const { return PlayerIndex; }

	UFUNCTION(BlueprintPure)
	int32 GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure)
	bool IsDead() const { return CurrentHealth <= 0; }

    UPROPERTY(BlueprintAssignable)
    FOnPlayerDied OnPlayerDied;

    // Server setters, clients receive the updated value in OnRep
    void SetTeamId(int32 NewTeamId);
    void SetTeam(ETeam NewTeam);
    void SetPlayerIndex(int32 PlayerIndex);
    void SetCurrentHealth(int32 NewHealth);
    void SetEpicAccountName(const FString& InName) { EpicAccountName = InName; }

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
    void OnRep_PlayerIndex();

    UFUNCTION()
    void OnRep_Team();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};