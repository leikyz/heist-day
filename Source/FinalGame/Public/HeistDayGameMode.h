#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeistDayGameState.h"
#include "HeistDayGameMode.generated.h"

/**
 * Server-only game loop for a 2-round 1v1 (Thief vs Employee) match.
 *
 * Flow
 * ────
 * 1. Both players connect → InitNewPlayer assigns ETeam + OriginalLobbyId.
 * 2. When all expected players are present → StartRound(1).
 * 3. 180 s timer fires → SnapshotRound1Scores → BetweenRounds (15 s).
 * 4. Between-rounds timer fires → SwapTeams + StartRound(2).
 * 5. 180 s timer fires → SnapshotRound2Scores → PostMatch → TravelPlayersToLobby.
 *
 * Points / scoring
 * ────────────────
 * The game loop is deliberately agnostic about *how* points are scored.
 * Your gameplay code should call  AddPointsToPlayer(PS, Delta)  on the server
 * any time a player earns or loses points. That function updates the transient
 * RoundScore on the PlayerState and is the only hook you need to implement later.
 *
 * Coins
 * ─────
 * Call  AddCoinsToPlayer(PS, Delta)  on the server during Round 1.
 * Coins survive the round swap and are visible in Round 2.
 */
UCLASS()
class FINALGAME_API AHeistDayGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AHeistDayGameMode();

    // ── Config (set in Blueprint defaults or DefaultGame.ini) ─────────────────

    /** Duration of each gameplay round in seconds. Default 180 (3 min). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Loop")
    float RoundDuration = 180.f;

    /** Break between rounds (end-of-round screen). Default 15 s. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Loop")
    float BetweenRoundsDuration = 15.f;

    /** Number of players expected before the first round starts. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Loop")
    int32 ExpectedPlayerCount = 2;

    // ── Public helpers — call these from your gameplay Blueprints/Components ──

    /**
     * Award points to a player during the current round.
     * Server-only; safe to call any time during Round1 or Round2.
     */
    UFUNCTION(BlueprintCallable, Category = "Game Loop")
    void AddPointsToPlayer(ANetworkPlayerState* PS, int32 Delta);

    /**
     * Award coins that persist across both rounds.
     * Server-only.
     */
    UFUNCTION(BlueprintCallable, Category = "Game Loop")
    void AddCoinsToPlayer(ANetworkPlayerState* PS, int32 Delta);

    // ── Overrides ─────────────────────────────────────────────────────────────

    virtual FString InitNewPlayer(APlayerController* NewPlayerController,
        const FUniqueNetIdRepl& UniqueId,
        const FString& Options,
        const FString& Portal) override;

    virtual void PostLogin(APlayerController* NewPlayer) override;

private:

    // ── Round management ──────────────────────────────────────────────────────

    void StartRound(int32 RoundNumber);
    void OnRoundTimerExpired();
    void OnBetweenRoundsTimerExpired();

    /** Reads current RoundScore from every PS and stores it for the given round. */
    void SnapshotRoundScores(int32 RoundNumber);

    /** Flips Thief→Employee and Employee→Thief for all connected players. */
    void SwapTeams();

    /** Resets per-round score counters on all PlayerStates (coins untouched). */
    void ResetRoundScores();

    /**
     * Groups players by OriginalLobbyId and calls ClientTravel on each
     * controller to send them back to the correct lobby map/URL.
     *
     * Players who shared a lobby arrive back together; solo players return alone.
     */
    void TravelPlayersToLobby();

    // ── Helpers ───────────────────────────────────────────────────────────────

    AHeistDayGameState* GetHeistGameState() const;

    /** Transient per-round score: reset each round, snapshotted at round end. */
    TMap<ANetworkPlayerState*, int32> CurrentRoundScores;

    int32 CurrentRound = 0;

    FTimerHandle RoundTimerHandle;
    FTimerHandle BetweenRoundsTimerHandle;
};