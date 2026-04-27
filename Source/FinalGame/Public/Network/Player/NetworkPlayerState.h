// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetworkPlayerState.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
    None     UMETA(DisplayName = "None"),
    Thief    UMETA(DisplayName = "Thief"),
    Employee UMETA(DisplayName = "Employee")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamChanged);

UCLASS()
class FINALGAME_API ANetworkPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category = "Team")
    ETeam GetTeam() const { return Team; }

    void SetTeam(ETeam NewTeam);

    UPROPERTY(BlueprintAssignable, Category = "Team")
    FOnTeamChanged OnTeamChanged;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;

    UPROPERTY(ReplicatedUsing = OnRep_Team)
    ETeam Team = ETeam::None;

    UFUNCTION()
    void OnRep_Team();
};