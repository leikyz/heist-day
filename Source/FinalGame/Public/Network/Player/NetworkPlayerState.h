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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScoreChanged);


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


    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetRound1Score() const { return Round1Score; }

    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetRound2Score() const { return Round2Score; }

    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetTotalScore() const { return Round1Score + Round2Score; }

    void SetRoundScore(int32 RoundNumber, int32 Score);

    UPROPERTY(BlueprintAssignable, Category = "Score")
    FOnScoreChanged OnScoreChanged;


    UFUNCTION(BlueprintPure, Category = "Coins")
    int32 GetCoins() const { return Coins; }

    void AddCoins(int32 Delta);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetOriginalLobbyId() const { return OriginalLobbyId; }

    void SetOriginalLobbyId(const FString& LobbyId);

protected:

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;


    UPROPERTY(ReplicatedUsing = OnRep_Team)
    ETeam Team = ETeam::None;

    UPROPERTY(ReplicatedUsing = OnRep_Score)
    int32 Round1Score = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Score)
    int32 Round2Score = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Score)
    int32 Coins = 0;

    UPROPERTY(Replicated)
    FString OriginalLobbyId;

private:

    UFUNCTION()
    void OnRep_Team();

    void OnRep_Score();
};