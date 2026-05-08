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
    if (!PS) return Super::ChoosePlayerStart_Implementation(Player);

    FName TargetTag = (PS->GetTeam() == ETeam::Thief)
        ? FName("Thief")
        : FName("Employee");

    // collect all valid starts for this team
    TArray<APlayerStart*> ValidStarts;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* Start = *It;
        if (Start->PlayerStartTag == TargetTag && !UsedPlayerStarts.Contains(Start))
            ValidStarts.Add(Start);
    }

    // sort by name so First always comes before Second
    ValidStarts.Sort([](const APlayerStart& A, const APlayerStart& B)
        {
            return A.GetName() < B.GetName();
        });

    if (ValidStarts.Num() > 0)
    {
        UsedPlayerStarts.Add(ValidStarts[0]);
        return ValidStarts[0];
    }

    UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Team = %d PlayerIndex = %d"),
        (int32)PS->GetTeam(), PS->GetPlayerIndex());

    return Super::ChoosePlayerStart_Implementation(Player);
}

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    ConnectedCount++;

    if (auto* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>())
    {
        PS->SetTeamId(ConnectedCount);

        if (ConnectedCount == 1 || ConnectedCount == 2)
        {
            PS->SetTeam(ETeam::Thief);
            ThiefCount++;
            PS->SetPlayerIndex(ThiefCount);
        }
        else
        {
            PS->SetTeam(ETeam::Employee);
            EmployeeCount++;
            PS->SetPlayerIndex(EmployeeCount);
        }
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