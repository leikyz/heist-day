#include "HeistDayGameState.h"
#include "Net/UnrealNetwork.h"

AHeistDayGameState::AHeistDayGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentMatchData = FMatchData();
}

void AHeistDayGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AHeistDayGameState, RemainingTime);
    DOREPLIFETIME(AHeistDayGameState, MatchPhase);
    DOREPLIFETIME(AHeistDayGameState, CurrentMatchData);
    DOREPLIFETIME(AHeistDayGameState, GlobalMuseumValue);
    DOREPLIFETIME(AHeistDayGameState, bIsAlarming);
}



void AHeistDayGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority()) return;
    if (RemainingTime <= 0.f) return;

    RemainingTime = FMath::Max(0.f, RemainingTime - DeltaSeconds);
}

void AHeistDayGameState::Server_SetIsAlarming(bool bNewIsAlarming)
{
    if (bIsAlarming != bNewIsAlarming)
    {
        bIsAlarming = bNewIsAlarming;

        if (HasAuthority() && GetNetMode() != NM_DedicatedServer)
        {
            OnRep_IsAlarming();
        }
    }
}

void AHeistDayGameState::Server_SetRemainingTime(float Seconds)
{
    RemainingTime = Seconds;
}

void AHeistDayGameState::Server_SetMatchPhase(EMatchPhase NewPhase)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameState] Server_SetMatchPhase called with new phase : %d"), static_cast<uint8>(NewPhase));
    MatchPhase = NewPhase;
}

void AHeistDayGameState::Server_SetMuseumValue(int32 NewValue)
{
    GlobalMuseumValue = NewValue;
}

void AHeistDayGameState::OnRep_MatchPhase()
{
    OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AHeistDayGameState::OnRep_RemainingTime()
{
    OnRemainingTimeChanged.Broadcast(RemainingTime);
}

void AHeistDayGameState::OnRep_GlobalMuseumValue()
{
    OnMuseumValueChanged.Broadcast(GlobalMuseumValue);
}

void AHeistDayGameState::OnRep_IsAlarming()
{
    OnIsAlarmingChanged.Broadcast(bIsAlarming);
}   


void AHeistDayGameState::OnRep_CurrentMatchData()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameState] OnRep_CurrentMatchData called on Client."));

    OnTeamValueChanged.Broadcast(CurrentMatchData.FirstTeam);
    OnTeamValueChanged.Broadcast(CurrentMatchData.SecondTeam);
}

FTeamData AHeistDayGameState::GetTeamDataById(int32 TeamId) const
{
    if (CurrentMatchData.FirstTeam.TeamId == TeamId)
    {
        return CurrentMatchData.FirstTeam;
    }

    if (CurrentMatchData.SecondTeam.TeamId == TeamId)
    {
        return CurrentMatchData.SecondTeam;
    }

    return FTeamData();
}

FTeamData AHeistDayGameState::GetOpposingTeamDataById(int32 TeamId) const
{
    if (CurrentMatchData.FirstTeam.TeamId == TeamId)
    {
        return CurrentMatchData.SecondTeam;
    }

    if (CurrentMatchData.SecondTeam.TeamId == TeamId)
    {
        return CurrentMatchData.FirstTeam;
    }

    return FTeamData();
}

void AHeistDayGameState::Server_SetThiefScore(int32 TeamId, int32 ScoreToAdd)
{
    FTeamData* ModifiedTeam = (CurrentMatchData.FirstTeam.TeamId == TeamId) ? &CurrentMatchData.FirstTeam :
        (CurrentMatchData.SecondTeam.TeamId == TeamId) ? &CurrentMatchData.SecondTeam : nullptr;

    if (ModifiedTeam != nullptr)
    {
        ModifiedTeam->ThiefScore += ScoreToAdd;
        OnTeamValueChanged.Broadcast(*ModifiedTeam);
    }
}


FTransform AHeistDayGameState::GetPlayerRespawnTransform(AController* Player)
{
    if (Player && Player->StartSpot.IsValid())
    {
        return Player->StartSpot->GetActorTransform();
    }


    if (Player && Player->GetPawn())
    {
        UE_LOG(LogTemp, Error, TEXT("[GetPlayerRespawnTransform] StartSpot introuvable pour %s !"), *Player->GetName());
        return Player->GetPawn()->GetActorTransform();
    }

    return FTransform::Identity;
}

bool AHeistDayGameState::GetMatchWinner(FTeamData& OutWinner)
{
	const FTeamData& FirstTeam = CurrentMatchData.FirstTeam;
    const FTeamData& SecondTeam = CurrentMatchData.SecondTeam;

    if (FirstTeam.ThiefScore > SecondTeam.ThiefScore)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Winner: Team %d"), FirstTeam.TeamId);
        OutWinner = FirstTeam;
        return true;
    }
    else if (SecondTeam.ThiefScore > FirstTeam.ThiefScore)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Winner: Team %d"), SecondTeam.TeamId);
        OutWinner = SecondTeam;
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Match Draw at %d"), FirstTeam.ThiefScore);
    return false;
}

bool AHeistDayGameState::GetMatchLooser(FTeamData& OutLooser)
{
    const FTeamData& FirstTeam = CurrentMatchData.FirstTeam;
    const FTeamData& SecondTeam = CurrentMatchData.SecondTeam;
    if (FirstTeam.ThiefScore < SecondTeam.ThiefScore)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Looser: Team %d"), FirstTeam.TeamId);
        OutLooser = FirstTeam;
        return true;
    }
    else if (SecondTeam.ThiefScore < FirstTeam.ThiefScore)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Looser: Team %d"), SecondTeam.TeamId);
        OutLooser = SecondTeam;
        return true;
    }
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Match Draw at %d"), FirstTeam.ThiefScore);
    return false;
}

void AHeistDayGameState::Server_SetEmployeeScore(int32 TeamId, int32 ScoreToAdd)
{
    FTeamData* ModifiedTeam = (CurrentMatchData.FirstTeam.TeamId == TeamId) ? &CurrentMatchData.FirstTeam :
        (CurrentMatchData.SecondTeam.TeamId == TeamId) ? &CurrentMatchData.SecondTeam : nullptr;

    if (ModifiedTeam != nullptr)
    {
        ModifiedTeam->EmployeeScore -= ScoreToAdd;
        OnTeamValueChanged.Broadcast(*ModifiedTeam);
    }
}