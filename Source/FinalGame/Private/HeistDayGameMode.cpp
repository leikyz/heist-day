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
        UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] ECHEC : Le PlayerState est NULL pour %s !"), *Player->GetName());
        return Super::ChoosePlayerStart_Implementation(Player);
    }

    FName TargetTag = (PS->GetTeam() == ETeam::Thief)
        ? FName("Thief")
        : FName("Employee");

    TArray<APlayerStart*> ValidStarts;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* Start = *It;
        if (Start->PlayerStartTag == TargetTag)
        {
            ValidStarts.Add(Start);
        }
    }

    ValidStarts.Sort([](const APlayerStart& A, const APlayerStart& B)
        {
            return A.GetName() < B.GetName();
        });

    if (ValidStarts.Num() > 0)
    {
        int32 SpawnIndex = FMath::Max(0, PS->GetPlayerIndex() - 1);
        SpawnIndex = SpawnIndex % ValidStarts.Num();

        UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] SUCCES : Assigné au spawn %s pour %s (PlayerIndex : %d)"),
            *ValidStarts[SpawnIndex]->GetName(), *Player->GetName(), PS->GetPlayerIndex());

        return ValidStarts[SpawnIndex];
    }

    UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] ERREUR : Aucun spawn trouvé pour l'équipe %s !"), *TargetTag.ToString());

    return Super::ChoosePlayerStart_Implementation(Player);
}

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    ConnectedCount++;

    if (auto* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>())
    {
        PS->SetTeamId(ConnectedCount);

        PS->SetTeam(ETeam::Thief);
        EmployeeCount++;

        PS->SetPlayerIndex(EmployeeCount);
    }

    Super::PostLogin(NewPlayer);

    if (ConnectedCount >= ExpectedPlayerCount)
    {
        if (CachedGameState)
        {
            CachedGameState->Server_SetMatchPhase(EMatchPhase::PreRound);
        }

        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound();
            }, 12.0f, false);
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