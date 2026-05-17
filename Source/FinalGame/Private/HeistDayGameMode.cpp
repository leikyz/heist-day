#include "HeistDayGameMode.h"
#include "TimerManager.h"
#include "HeistDayPlayerState.h"
#include "HeistDayGameState.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Network/Client/EOSMatchmakingSubsystem.h>

void AHeistDayGameMode::BeginPlay()
{
    Super::BeginPlay();
    CachedGameState = GetGameState<AHeistDayGameState>();

    SaveCarryablesInitialState();
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

    //if (!CheckAllThiefDead())
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Thiefs are still alive, round continues."));

    //}
    //else
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("[GameMode] All thiefs are dead, round should end."));
    //    GetWorldTimerManager().ClearTimer(RoundTimerHandle);
    //    // Handle end of round logic here
    //}
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
    int32 AssignedTeamId = (ConnectedCount % 2 != 0) ? 1 : 2;

   /* UGameInstance* GI = GetGameInstance();
    if (GI)
    {
        UEOSMatchmakingSubsystem* MatchmakingSubsystem = GI->GetSubsystem<UEOSMatchmakingSubsystem>();

        if (MatchmakingSubsystem)
        {
            if (MatchmakingSubsystem->PendingTeamID != -1)
                AssignedTeamId = MatchmakingSubsystem->PendingTeamID;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] EOSMatchmakingSubsystem not found, using default team assignment."));
        }
    }*/

    PS->SetTeamId(AssignedTeamId);

    if (AssignedTeamId == 1)
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

    FTeamData& TargetTeamData = (AssignedTeamId == 1)
        ? CachedGameState->CurrentMatchData.FirstTeam
        : CachedGameState->CurrentMatchData.SecondTeam;

    CachedGameState->CurrentMatchData.FirstTeam.EmployeeScore = CachedGameState->GetMuseumValue();
    CachedGameState->CurrentMatchData.SecondTeam.EmployeeScore = CachedGameState->GetMuseumValue();


    if (TargetTeamData.TeamId == 0)
    {
        TargetTeamData.TeamId = AssignedTeamId;
        TargetTeamData.Team = PS->GetTeam();
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Initialisation de la Team %d dans MatchData"), AssignedTeamId);
    }

    TargetTeamData.Players.Add(PS);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Joueur %s ajouté à la structure MatchData de la Team %d"), *PS->GetPlayerName(), AssignedTeamId);

    Super::PostLogin(NewPlayer);

    if (AActor* InitialStart = ChoosePlayerStart(NewPlayer))
    {
        NewPlayer->StartSpot = InitialStart;
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] StartSpot initialisé à %s pour %s"), *InitialStart->GetName(), *NewPlayer->GetName());
    }

   
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
            }, 14.0f, false);
    }
}
void AHeistDayGameMode::ResetAllPlayersHealth()
{
    if (!CachedGameState) return;

    for (APlayerState* PS : CachedGameState->PlayerArray)
    {
        AHeistDayPlayerState* HeistPS = Cast<AHeistDayPlayerState>(PS);
        if (HeistPS)
        {
            HeistPS->SetCurrentHealth(AHeistDayPlayerState::MaxHealth);

            UE_LOG(LogTemp, Log, TEXT("[GameMode] Santé réinitialisée pour : %s"), *HeistPS->GetPlayerName());
        }
    }
}

void AHeistDayGameMode::TeleportPlayersToNewSpawns()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->GetPawn())
        {
            AActor* NewStart = ChoosePlayerStart(PC);

            if (NewStart)
            {
                FVector SpawnLocation = NewStart->GetActorLocation();
                FRotator SpawnRotation = NewStart->GetActorRotation();

                PC->GetPawn()->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

                PC->StartSpot = NewStart;

                if (auto* Movement = PC->GetPawn()->FindComponentByClass<UCharacterMovementComponent>())
                {
                    Movement->StopMovementImmediately();
                }
            }
        }
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
        CachedGameState->Server_SetRemainingTime(6.0f);
        CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRoundEnd);

        SwapAllTeamsRoles();

        CachedGameState->Server_SetIsAlarming(false);

        GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
            {
                //SwapAllTeamsRoles();

                TeleportPlayersToNewSpawns();

                ResetAllPlayersHealth();

                ResetCarryables();

                CachedGameState->Server_SetRemainingTime(14.0f);
                CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRoundStart);

                GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
                    {
                        StartRound(2);

                    }, 14.0f, false);

            }, 6.0f, false);
        break;
    }
    case EMatchPhase::SecondRound:
    {
        CachedGameState->Server_SetRemainingTime(6.0f);
        CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRoundEnd);

        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Second round ended. Match should end or restart after delay."));

        GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
            {
                CachedGameState->Server_SetMatchPhase(EMatchPhase::MatchEnd);
            }, 6.0f, false);

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

    int32 TempCount = ThiefCount;
    ThiefCount = EmployeeCount;
    EmployeeCount = TempCount;

    CachedGameState->CurrentMatchData = NewMatchData;

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

void AHeistDayGameMode::SaveCarryablesInitialState()
{
    InitialCarryablesData.Empty();

    if (CarryableBaseClass)
    {
        for (TActorIterator<AActor> It(GetWorld(), CarryableBaseClass); It; ++It)
        {
            if (AActor* Item = *It)
            {
                FCarryableSpawnData Data;
                Data.CarryableClass = Item->GetClass();
                Data.InitialTransform = Item->GetActorTransform();
                InitialCarryablesData.Add(Data);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] CarryableBaseClass n'est pas assigné ! Mets BP_CarryableBase dans le Blueprint du GameMode."));
    }

    if (KeycardBaseClass)
    {
        for (TActorIterator<AActor> It(GetWorld(), KeycardBaseClass); It; ++It)
        {
            if (AActor* Item = *It)
            {
                FCarryableSpawnData Data;
                Data.CarryableClass = Item->GetClass();
                Data.InitialTransform = Item->GetActorTransform();
                InitialCarryablesData.Add(Data);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] KeycardBaseClass n'est pas assigné ! Mets BP_Keycard_Pickup dans le Blueprint du GameMode."));
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] %d objets (Carryables + Keycards) sauvegardés au démarrage."), InitialCarryablesData.Num());
}

void AHeistDayGameMode::ResetCarryables()
{
    if (CarryableBaseClass)
    {
        for (TActorIterator<AActor> It(GetWorld(), CarryableBaseClass); It; ++It)
        {
            if (AActor* Item = *It)
            {
                Item->Destroy();
            }
        }
    }

    if (KeycardBaseClass)
    {
        for (TActorIterator<AActor> It(GetWorld(), KeycardBaseClass); It; ++It)
        {
            if (AActor* Item = *It)
            {
                Item->Destroy();
            }
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (const FCarryableSpawnData& Data : InitialCarryablesData)
    {
        if (Data.CarryableClass)
        {
            GetWorld()->SpawnActor<AActor>(Data.CarryableClass, Data.InitialTransform, SpawnParams);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] %d objets (Carryables + Keycards) réinitialisés !"), InitialCarryablesData.Num());
}
