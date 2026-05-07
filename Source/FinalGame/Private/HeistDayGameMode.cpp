#include "HeistDayGameMode.h"
#include "TimerManager.h"
#include <HeistDayPlayerState.h>


void AHeistDayGameMode::BeginPlay()
{
    Super::BeginPlay();
    CachedGameState = GetGameState<AHeistDayGameState>();
}


void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    ConnectedCount++;

    if (auto* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>())
    {
        PS->SetTeamId(ConnectedCount); // first player = 1, second = 2
        PS->SetTeam(static_cast<ETeam>(ConnectedCount));
    }

    if (ConnectedCount >= ExpectedPlayerCount)
    {
        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound();
            }, 2.0f, false);
    }
}

void AHeistDayGameMode::StartRound()
{
    if (!CachedGameState) return;

    CachedGameState->Server_SetRemainingTime(RoundDuration);
    CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRound);

    GetWorldTimerManager().SetTimer(RoundTimerHandle,
        this, &AHeistDayGameMode::OnRoundTimerExpired,
        RoundDuration, false);

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round started — %.0f s"), RoundDuration);
}

void AHeistDayGameMode::OnRoundTimerExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round over."));
}