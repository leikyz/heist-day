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

UCLASS()
class FINALGAME_API AHeistDayPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure)
    int32 GetTeamId() const { return TeamId; }

    // Server setters, clients receive the updated value in OnR
    void SetTeamId(int32 NewTeamId);
    void SetTeam(ETeam NewTeam);
private:
    UPROPERTY(ReplicatedUsing = OnRep_TeamId)
    int32 TeamId = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Team)
    ETeam Team = ETeam::None;

    UFUNCTION()
    void OnRep_TeamId();

    UFUNCTION()
    void OnRep_Team();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};