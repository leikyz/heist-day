#include "HeistDayGameMode.h"
#include "Network/Player/NetworkPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AHeistDayGameMode::AHeistDayGameMode()
{
    PlayerStateClass = ANetworkPlayerState::StaticClass();
    GameStateClass = AHeistDayGameState::StaticClass();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitNewPlayer — reads ?Team=N and ?Lobby=<id> from the join URL
// ─────────────────────────────────────────────────────────────────────────────

FString AHeistDayGameMode::InitNewPlayer(APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

    const int32 TeamInt = UGameplayStatics::GetIntOption(Options, TEXT("Team"), -1);
    const FString LobbyId = UGameplayStatics::ParseOption(Options, TEXT("Lobby"));

    UE_LOG(LogTemp, Warning,
        TEXT("[GameMode] InitNewPlayer Id=%s Team=%d LobbyId=%s"),
        *UniqueId.ToString(), TeamInt, *LobbyId);

    if (auto* PS = NewPlayerController
        ? NewPlayerController->GetPlayerState<ANetworkPlayerState>()
        : nullptr)
    {
        if (TeamInt >= 0)
            PS->SetTeam(static_cast<ETeam>(TeamInt));
        else
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] No ?Team= in URL for %s"), *UniqueId.ToString());

        if (!LobbyId.IsEmpty())
            PS->SetOriginalLobbyId(LobbyId);
        else
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] No ?Lobby= in URL for %s"), *UniqueId.ToString());
    }

    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// PostLogin — start the round once enough players have arrived
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    const int32 ConnectedCount = GetNumPlayers();

    UE_LOG(LogTemp, Warning,
        TEXT("[GameMode] PostLogin — connected %d / %d"),
        ConnectedCount, ExpectedPlayerCount);

    if (ConnectedCount >= ExpectedPlayerCount && CurrentRound == 0)
    {
        // Small delay so both clients finish loading before the timer starts.
        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound(1);
            }, 2.0f, false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// StartRound
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::StartRound(int32 RoundNumber)
{
    CurrentRound = RoundNumber;
    ResetRoundScores();

    AHeistDayGameState* GS = GetHeistGameState();
    if (!GS) return;

    const EMatchPhase Phase = (RoundNumber == 1)
        ? EMatchPhase::Round1
        : EMatchPhase::Round2;

    GS->Server_SetMatchPhase(Phase);
    GS->Server_SetRemainingTime(RoundDuration);
    GS->Multicast_NotifyRoundStarted(RoundNumber);

    // Primary game timer — fires when the round ends.
    GetWorldTimerManager().SetTimer(RoundTimerHandle,
        this, &AHeistDayGameMode::OnRoundTimerExpired,
        RoundDuration, false);

    UE_LOG(LogTemp, Warning,
        TEXT("[GameMode] Round %d started (%.0f s)"), RoundNumber, RoundDuration);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnRoundTimerExpired — called when the 3-min timer fires
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::OnRoundTimerExpired()
{
    AHeistDayGameState* GS = GetHeistGameState();
    if (!GS) return;

    SnapshotRoundScores(CurrentRound);

    if (CurrentRound == 1)
    {
        // ── Between rounds ────────────────────────────────────────────────────
        GS->Server_SetMatchPhase(EMatchPhase::BetweenRounds);
        GS->Server_SetRemainingTime(BetweenRoundsDuration);
        GS->Multicast_NotifyRoundEnded(GS->GetRound1Result());

        GetWorldTimerManager().SetTimer(BetweenRoundsTimerHandle,
            this, &AHeistDayGameMode::OnBetweenRoundsTimerExpired,
            BetweenRoundsDuration, false);

        UE_LOG(LogTemp, Warning,
            TEXT("[GameMode] Round 1 over. Between-rounds screen (%.0f s)"), BetweenRoundsDuration);
    }
    else
    {
        // ── Match over ────────────────────────────────────────────────────────
        GS->Server_SetMatchPhase(EMatchPhase::PostMatch);
        GS->Multicast_NotifyMatchEnded(GS->GetRound1Result(), GS->GetRound2Result());

        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round 2 over. Match ended."));

        // Travel players back after a short delay so the final screen shows briefly
        // on the server before the map changes. The real final screen timer is client-
        // side (the HUD component handles it); the server just waits a safe margin.
        FTimerHandle FinalDelay;
        GetWorldTimerManager().SetTimer(FinalDelay, [this]()
            {
                TravelPlayersToLobby();
            }, 15.0f, false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OnBetweenRoundsTimerExpired — swap teams and start round 2
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::OnBetweenRoundsTimerExpired()
{
    SwapTeams();
    StartRound(2);
}

// ─────────────────────────────────────────────────────────────────────────────
// SnapshotRoundScores — copy CurrentRoundScores into GameState
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::SnapshotRoundScores(int32 RoundNumber)
{
    AHeistDayGameState* GS = GetHeistGameState();
    if (!GS) return;

    FRoundResult Result;

    for (auto& Pair : CurrentRoundScores)
    {
        ANetworkPlayerState* PS = Pair.Key;
        if (!IsValid(PS)) continue;

        const int32 Score = Pair.Value;
        PS->SetRoundScore(RoundNumber, Score);

        if (PS->GetTeam() == ETeam::Thief)         Result.ThiefScore += Score;
        else if (PS->GetTeam() == ETeam::Employee)  Result.EmployeeScore += Score;
    }

    if (RoundNumber == 1) GS->Server_SetRound1Result(Result);
    else                   GS->Server_SetRound2Result(Result);

    UE_LOG(LogTemp, Warning,
        TEXT("[GameMode] Round %d snapshot — Thief:%d  Employee:%d"),
        RoundNumber, Result.ThiefScore, Result.EmployeeScore);
}

// ─────────────────────────────────────────────────────────────────────────────
// SwapTeams — Thief→Employee, Employee→Thief
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::SwapTeams()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (auto* PC = It->Get())
        {
            if (auto* PS = PC->GetPlayerState<ANetworkPlayerState>())
            {
                const ETeam Current = PS->GetTeam();
                if (Current == ETeam::Thief)    PS->SetTeam(ETeam::Employee);
                else if (Current == ETeam::Employee)  PS->SetTeam(ETeam::Thief);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Teams swapped."));
}

// ─────────────────────────────────────────────────────────────────────────────
// ResetRoundScores — keep coins, zero out the round transient counters
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::ResetRoundScores()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (auto* PC = It->Get())
        {
            if (auto* PS = PC->GetPlayerState<ANetworkPlayerState>())
            {
                CurrentRoundScores.Add(PS, 0);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AddPointsToPlayer — called by gameplay code to score points
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::AddPointsToPlayer(ANetworkPlayerState* PS, int32 Delta)
{
    if (!IsValid(PS)) return;

    int32& Score = CurrentRoundScores.FindOrAdd(PS);
    Score = FMath::Max(0, Score + Delta);

    UE_LOG(LogTemp, Verbose,
        TEXT("[GameMode] Points: Player=%s Delta=%d NewScore=%d"),
        *PS->GetPlayerName(), Delta, Score);
}

// ─────────────────────────────────────────────────────────────────────────────
// AddCoinsToPlayer — persists across rounds
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::AddCoinsToPlayer(ANetworkPlayerState* PS, int32 Delta)
{
    if (!IsValid(PS)) return;
    PS->AddCoins(Delta);
}

// ─────────────────────────────────────────────────────────────────────────────
// TravelPlayersToLobby
// ─────────────────────────────────────────────────────────────────────────────

void AHeistDayGameMode::TravelPlayersToLobby()
{
    // Build a map of LobbyId → list of PlayerControllers so players who shared
    // a lobby all go to the same destination URL.
    TMap<FString, TArray<APlayerController*>> LobbyGroups;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC) continue;

        auto* PS = PC->GetPlayerState<ANetworkPlayerState>();
        const FString LobbyId = PS ? PS->GetOriginalLobbyId() : FString();

        if (LobbyId.IsEmpty())
        {
            // Fallback: send to the default lobby map.
            UE_LOG(LogTemp, Warning,
                TEXT("[GameMode] Player %s has no LobbyId — travelling to default lobby."),
                PS ? *PS->GetPlayerName() : TEXT("Unknown"));
            PC->ClientTravel(TEXT("/Game/Maps/Lobby?listen"), TRAVEL_Absolute);
        }
        else
        {
            LobbyGroups.FindOrAdd(LobbyId).Add(PC);
        }
    }

    // Each unique lobby destination gets a single ClientTravel call per PC.
    for (auto& Group : LobbyGroups)
    {
        const FString& LobbyId = Group.Key;

        // The travel URL re-uses the same ?Lobby= that brought them here.
        // Adjust this to match your actual lobby map path / EOS lobby connect string.
        const FString TravelURL = FString::Printf(
            TEXT("/Game/Maps/Lobby?Lobby=%s?listen"), *LobbyId);

        UE_LOG(LogTemp, Warning,
            TEXT("[GameMode] Travelling %d player(s) to lobby '%s'"),
            Group.Value.Num(), *LobbyId);

        for (APlayerController* PC : Group.Value)
        {
            PC->ClientTravel(TravelURL, TRAVEL_Absolute);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GetHeistGameState — typed accessor
// ─────────────────────────────────────────────────────────────────────────────

AHeistDayGameState* AHeistDayGameMode::GetHeistGameState() const
{
    return GetGameState<AHeistDayGameState>();
}