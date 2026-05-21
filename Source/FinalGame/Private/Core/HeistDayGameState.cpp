#include "Core/HeistDayGameState.h"
#include "Net/UnrealNetwork.h"
#include "Core/HeistDayGameMode.h"

#pragma region Lifecycle & Replication

AHeistDayGameState::AHeistDayGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentMatchData = FMatchData();
}

void AHeistDayGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
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

#pragma endregion

#pragma region Server API

void AHeistDayGameState::Server_SetIsAlarming(bool bNewIsAlarming)
{
	if (bIsAlarming != bNewIsAlarming)
	{
		bIsAlarming = bNewIsAlarming;

		if (HasAuthority())
		{
			if (bIsAlarming)
			{
				float AlarmTime = 60.0f;

				if (RemainingTime > AlarmTime)
				{
					Server_SetRemainingTime(AlarmTime);

					AHeistDayGameMode* GM = Cast<AHeistDayGameMode>(GetWorld()->GetAuthGameMode());
					if (GM)
					{
						GM->SetAlarmTimer(AlarmTime);
					}
				}
			}

			if (GetNetMode() != NM_DedicatedServer)
			{
				OnRep_IsAlarming();
			}
		}
	}
}

void AHeistDayGameState::Server_SetRemainingTime(float Seconds)
{
	RemainingTime = Seconds;
}

void AHeistDayGameState::Server_SetMatchPhase(EMatchPhase NewPhase)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameState] Server_SetMatchPhase called with new phase: %d"), static_cast<uint8>(NewPhase));
	MatchPhase = NewPhase;
}

void AHeistDayGameState::Server_SetMuseumValue(int32 NewValue)
{
	GlobalMuseumValue = NewValue;
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

#pragma endregion

#pragma region OnRep Callbacks

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

#pragma endregion

#pragma region Match Data Helpers

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

FTransform AHeistDayGameState::GetPlayerRespawnTransform(AController* Player)
{
	if (Player && Player->StartSpot.IsValid())
	{
		return Player->StartSpot->GetActorTransform();
	}

	if (Player && Player->GetPawn())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameState] StartSpot not found for %s!"), *Player->GetName());
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
		UE_LOG(LogTemp, Log, TEXT("[GameState] Winner: Team %d"), FirstTeam.TeamId);
		OutWinner = FirstTeam;
		return true;
	}
	else if (SecondTeam.ThiefScore > FirstTeam.ThiefScore)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameState] Winner: Team %d"), SecondTeam.TeamId);
		OutWinner = SecondTeam;
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameState] Match Draw at %d points"), FirstTeam.ThiefScore);
	return false;
}

bool AHeistDayGameState::GetMatchLooser(FTeamData& OutLooser)
{
	const FTeamData& FirstTeam = CurrentMatchData.FirstTeam;
	const FTeamData& SecondTeam = CurrentMatchData.SecondTeam;

	if (FirstTeam.ThiefScore < SecondTeam.ThiefScore)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameState] Loser: Team %d"), FirstTeam.TeamId);
		OutLooser = FirstTeam;
		return true;
	}
	else if (SecondTeam.ThiefScore < FirstTeam.ThiefScore)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameState] Loser: Team %d"), SecondTeam.TeamId);
		OutLooser = SecondTeam;
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameState] Match Draw at %d points"), FirstTeam.ThiefScore);
	return false;
}

#pragma endregion