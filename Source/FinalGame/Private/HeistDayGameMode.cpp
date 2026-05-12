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
        UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] Failed : PlayerState is NULL for %s !"), *Player->GetName());
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
        UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Success : Spawn %s assigned to %s (PlayerIndex : %d)"),
            *ValidStarts[SpawnIndex]->GetName(), *Player->GetName(), PS->GetPlayerIndex());

        return ValidStarts[SpawnIndex];
    }

    UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] Error: No valid spawn found for team %s !"), *TargetTag.ToString());

    return Super::ChoosePlayerStart_Implementation(Player);
}

bool AHeistDayGameMode::CheckAllThiefDead()
{
    if (!CachedGameState) return false;
    for (APlayerState* PS : CachedGameState->PlayerArray)
    {
        AHeistDayPlayerState* HeistPS = Cast<AHeistDayPlayerState>(PS);
        if (HeistPS && HeistPS->GetTeam() == ETeam::Thief && !HeistPS->IsDead())
        {
            return false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] All thiefs are dead!"));
    return true;
}

void AHeistDayGameMode::HandlePlayerDamage(AController* Victim, AController* Attacker,float DamageAmount)
{
    AHeistDayPlayerState* VictimPS = Victim->GetPlayerState<AHeistDayPlayerState>();
    AHeistDayPlayerState* AttackerPS = Attacker->GetPlayerState<AHeistDayPlayerState>();
	AHeistDayGameState* GS = GetGameState<AHeistDayGameState>();
    if (!VictimPS) return;

    float NewHealth = VictimPS->GetCurrentHealth() - DamageAmount;
    VictimPS->SetCurrentHealth(FMath::Max(0.f, NewHealth));

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s took %.1f damage from %s, new health: %d"),
		*Victim->GetName(), DamageAmount, Attacker ? *Attacker->GetName() : TEXT("Environment"), VictimPS->GetCurrentHealth());

    if (VictimPS->IsDead())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s has died!"), *Victim->GetName());

        if (GS->MatchPhase == EMatchPhase::FirstRound)
        {
            VictimPS->AddDeath(1);
            ;
            if (AttackerPS)
            {
                AttackerPS->AddKill(1);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s killed by environment or unknown source."), *Victim->GetName());
            }
        }
        else if (GS->MatchPhase == EMatchPhase::SecondRound)
        {
            VictimPS->AddDeath(2);
            ;
            if (AttackerPS)
            {
                AttackerPS->AddKill(2);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s killed by environment or unknown source."), *Victim->GetName());
			}
        }

    }

    if (!CheckAllThiefDead())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Thiefs are still alive, round continues."));

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] All thiefs are dead, round should end."));
        GetWorldTimerManager().ClearTimer(RoundTimerHandle);
        // Handle end of round logic here (e.g., declare winner, reset level, etc.)
    }
}

void AHeistDayGameMode::HandleChangePlayerHealthValue(AController* Victim, int32 NewHealth)
{
    AHeistDayPlayerState* PS = Victim->GetPlayerState<AHeistDayPlayerState>();
    if (!PS) return;

    if (NewHealth < 0 || NewHealth > AHeistDayPlayerState::MaxHealth)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleChangePlayerHealthValue] Try to set invalid health value (%d) for %s"), NewHealth, *Victim->GetName());
        return;
	}

    PS->SetCurrentHealth(NewHealth);
}

//void AHeistDayGameMode::HandlePlayerDeath(AController* Victim)
//{
//    
//}

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
    ConnectedCount++;

    AHeistDayPlayerState* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>();
    if (!PS || !CachedGameState)
    {
        Super::PostLogin(NewPlayer);
        return;
    }

    // 1. Déterminer l'ID d'équipe (1 ou 2)
    int32 AssignedTeamId = (ConnectedCount % 2 != 0) ? 2 : 2;
    PS->SetTeamId(AssignedTeamId);

    // 2. Assigner l'équipe et l'index (Logique de base)
    if (AssignedTeamId == 1)
    {
        PS->SetTeam(ETeam::Thief);
        ThiefCount++;
        PS->SetPlayerIndex(ThiefCount);
    }
    else
    {
        PS->SetTeam(ETeam::Thief);
        EmployeeCount++;
        PS->SetPlayerIndex(EmployeeCount);
    }

    FTeamData& TargetTeamData = (AssignedTeamId == 1)
        ? CachedGameState->CurrentMatchData.FirstTeam
        : CachedGameState->CurrentMatchData.SecondTeam;

    if (TargetTeamData.TeamId == 0)
    {
        TargetTeamData.TeamId = AssignedTeamId;
        TargetTeamData.Team = PS->GetTeam();
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Initialisation de la Team %d dans MatchData"), AssignedTeamId);
    }

    TargetTeamData.Players.Add(PS);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Joueur %s ajouté à la structure MatchData de la Team %d"), *PS->GetPlayerName(), AssignedTeamId);

    Super::PostLogin(NewPlayer);

  /*  if (ConnectedCount >= ExpectedPlayerCount)
    {
        CachedGameState->Server_SetMatchPhase(EMatchPhase::PreRound);
        CachedGameState->Server_SetRemainingTime(12.0f);

        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]() { StartRound(1); }, 12.0f, false);
    }*/
}
void AHeistDayGameMode::StartRound(int roundNumber)
{
    UsedPlayerStarts.Empty();

    if (!CachedGameState) return;

    CachedGameState->Server_SetRemainingTime(RoundDuration);

    switch (roundNumber)
    {
    case 1:
        CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRound);
		break;
	case 2:
        CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRound);
        break;
    default:
        CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRound);
        break;
    }

    GetWorldTimerManager().SetTimer(RoundTimerHandle,
        this, &AHeistDayGameMode::OnRoundTimerExpired,
        RoundDuration, false);

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round #%d started — %.0f s"), roundNumber, RoundDuration);
}

void AHeistDayGameMode::OnClientReady()
{
    ReadyPlayersCount++;
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Un client est prêt ! (%d/%d)"), ReadyPlayersCount, ExpectedPlayerCount);

    if (ReadyPlayersCount == ExpectedPlayerCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Tous les joueurs sont prêts ! Lancement du PreRound..."));

        if (CachedGameState)
        {
            CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRoundStart);
        }

        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound(1); // Always first round
            }, 12.0f, false);
    }
}

void AHeistDayGameMode::OnRoundTimerExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round over."));

    if (CachedGameState == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] CachedGameState is null in OnRoundTimerExpired!"));
        return;
    }

    switch (CachedGameState->GetMatchPhase())
    {
    case EMatchPhase::FirstRound:
    {
        SwapAllTeamsRoles();

        CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRoundEnd);

        UE_LOG(LogTemp, Warning, TEXT("[GameMode] First round ended. Starting second round after delay."));

	
        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                StartRound(2); // Always second round
            }, 14.0f, false);

        break;
    } 

    case EMatchPhase::SecondRound:
    {
        CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRoundEnd);

        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Second round ended. Match should end or restart after delay."));

        FTimerHandle StartDelay;
        GetWorldTimerManager().SetTimer(StartDelay, [this]()
            {
                CachedGameState->Server_SetMatchPhase(EMatchPhase::MatchEnd);
            }, 15.0f, false);

        break;
    }

    default:
        break;
    }
}

void AHeistDayGameMode::AwardThiefScore(int32 TeamId, int32 ScoreToAdd)
{
    FTeamData* ModifiedTeam = nullptr;

    if (CachedGameState)
    {
        CachedGameState->Server_SetThiefScore(TeamId, ScoreToAdd);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] L'arbitre a accordé %d points Thief à la team %d"), ScoreToAdd, TeamId);

        AwardEmployeeScore(CachedGameState->GetOpposingTeamDataById(TeamId).TeamId, ScoreToAdd);
    }
}
void AHeistDayGameMode::SwapAllTeamsRoles()
{
    if (!CachedGameState) return;

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] SwapAllTeamsRoles: Début du changement de rôle."));

    // Copie de la structure depuis le GameState
    FMatchData NewMatchData = CachedGameState->CurrentMatchData;

    auto SwapLogic = [](FTeamData& TeamData)
        {
            FString OldRoleStr = (TeamData.Team == ETeam::Thief) ? TEXT("Thief") : TEXT("Employee");

            TeamData.Team = (TeamData.Team == ETeam::Thief) ? ETeam::Employee : ETeam::Thief;

            FString NewRoleStr = (TeamData.Team == ETeam::Thief) ? TEXT("Thief") : TEXT("Employee");

            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Team %d Swap: %s -> %s"), TeamData.TeamId, *OldRoleStr, *NewRoleStr);

            int32 PlayerCount = 0;
            for (AHeistDayPlayerState* PS : TeamData.Players)
            {
                if (IsValid(PS))
                {
                    PS->SetTeam(TeamData.Team);
                    PlayerCount++;
                    UE_LOG(LogTemp, Log, TEXT("[GameMode] PlayerState %s mis à jour avec le rôle %s"), *PS->GetName(), *NewRoleStr);
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GameMode] %d joueurs mis à jour pour la Team %d"), PlayerCount, TeamData.TeamId);
        };

    SwapLogic(NewMatchData.FirstTeam);
    SwapLogic(NewMatchData.SecondTeam);

    // Swap des compteurs internes du GameMode
    int32 TempCount = ThiefCount;
    ThiefCount = EmployeeCount;
    EmployeeCount = TempCount;

    // Modification directe du struct dans le GameState (possible car GameMode est friend)
    CachedGameState->CurrentMatchData = NewMatchData;

    // Broadcast local sur le serveur
    CachedGameState->OnTeamValueChanged.Broadcast(CachedGameState->CurrentMatchData.FirstTeam);
    CachedGameState->OnTeamValueChanged.Broadcast(CachedGameState->CurrentMatchData.SecondTeam);

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] SwapAllTeamsRoles: Terminé et broadcasté localement."));
}
void AHeistDayGameMode::AwardEmployeeScore(int32 TeamId, int32 ScoreToAdd)
{
    FTeamData* ModifiedTeam = nullptr;

    if (CachedGameState)
    {
        CachedGameState->Server_SetEmployeeScore(TeamId, ScoreToAdd);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] L'arbitre a accordé %d points Thief à la team %d"), ScoreToAdd, TeamId);
    }
}