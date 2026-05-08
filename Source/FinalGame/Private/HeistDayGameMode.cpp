#include "HeistDayGameMode.h"
#include "TimerManager.h"
#include "HeistDayPlayerState.h"
#include "HeistDayGameState.h"
#include "EngineUtils.h"

void AHeistDayGameMode::BeginPlay()
{
    Super::BeginPlay();
    CachedGameState = GetGameState<AHeistDayGameState>();
}

AActor* AHeistDayGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AHeistDayPlayerState* PS = Player->GetPlayerState<AHeistDayPlayerState>();
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] No PlayerState!"));
        return Super::ChoosePlayerStart_Implementation(Player);
    }

    UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Team = %d"), (int32)PS->GetTeam());

    FName TargetTag = (PS->GetTeam() == ETeam::Thief)
        ? FName("Thief")
        : FName("Employee");

    UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Looking for tag = %s"), *TargetTag.ToString());

    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* Start = *It;
        UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Found start with tag = %s"), *Start->PlayerStartTag.ToString());
        if (Start->PlayerStartTag == TargetTag && !UsedPlayerStarts.Contains(Start))
        {
            UsedPlayerStarts.Add(Start);
            return Start;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] No valid start found!"));
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    ConnectedCount++;

    if (auto* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>())
    {
        PS->SetTeamId(ConnectedCount);
        PS->SetTeam(ConnectedCount == 1 ? ETeam::Thief : ETeam::Employee);
        UE_LOG(LogTemp, Warning, TEXT("[PostLogin] PS valid, Team set to %d"), (int32)PS->GetTeam());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PostLogin] Cast to AHeistDayPlayerState FAILED"));
    }

    APlayerController* PC = NewPlayer;
    FTimerHandle RespawnDelay;
    GetWorldTimerManager().SetTimer(RespawnDelay, [this, PC]()
        {
            if (IsValid(PC))
                RestartPlayer(PC);
        }, 0.2f, false);

    if (ConnectedCount >= ExpectedPlayerCount)
    {
        FTimerHandle PhaseDelay;
        GetWorldTimerManager().SetTimer(PhaseDelay, [this]()
            {
                if (CachedGameState)
                    CachedGameState->Server_SetMatchPhase(EMatchPhase::PreRound);
            }, 1.0f, false);

        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound();
            }, 2.0f, false);
    }
}

void AHeistDayGameMode::StartRound()
{
    UsedPlayerStarts.Empty();

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