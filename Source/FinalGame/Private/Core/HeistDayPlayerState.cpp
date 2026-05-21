#include "Core/HeistDayPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Core/HeistDayGameMode.h"

#pragma region Lifecycle & Replication

AHeistDayPlayerState::AHeistDayPlayerState()
{
	FirstRound = FRoundData();
	SecondRound = FRoundData();
}

void AHeistDayPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistDayPlayerState, TeamId);
	DOREPLIFETIME(AHeistDayPlayerState, Team);
	DOREPLIFETIME(AHeistDayPlayerState, PlayerIndex);
	DOREPLIFETIME(AHeistDayPlayerState, CurrentHealth);
	DOREPLIFETIME(AHeistDayPlayerState, EpicAccountName);
	DOREPLIFETIME(AHeistDayPlayerState, FirstRound);
	DOREPLIFETIME(AHeistDayPlayerState, SecondRound);
}

#pragma endregion

#pragma region Stats Modifiers

void AHeistDayPlayerState::AddInterception(int32 RoundNumber)
{
	if (HasAuthority())
	{
		if (RoundNumber == 1)
			FirstRound.InterceptionsCount++;
		else if (RoundNumber == 2)
			SecondRound.InterceptionsCount++;

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddInterception to player %d for Round %d. Total interceptions: %d"), GetPlayerId(), RoundNumber, (RoundNumber == 1) ? FirstRound.InterceptionsCount : SecondRound.InterceptionsCount);

		OnStatsChanged.Broadcast(this);
	}
}

void AHeistDayPlayerState::AddRobbed(int32 RoundNumber)
{
	if (HasAuthority())
	{
		if (RoundNumber == 1)
			FirstRound.robbedCount++;
		else if (RoundNumber == 2)
			SecondRound.robbedCount++;

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddRobbed to player %d for Round %d. Total robbed: %d"), GetPlayerId(), RoundNumber, (RoundNumber == 1) ? FirstRound.robbedCount : SecondRound.robbedCount);

		OnStatsChanged.Broadcast(this);
	}
}

void AHeistDayPlayerState::AddKill(int32 RoundNumber)
{
	if (HasAuthority())
	{
		if (RoundNumber == 1)
			FirstRound.KillsCount++;
		else if (RoundNumber == 2)
			SecondRound.KillsCount++;

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddKill to player %d for Round %d. Total kills: %d"), GetPlayerId(), RoundNumber, (RoundNumber == 1) ? FirstRound.KillsCount : SecondRound.KillsCount);

		OnStatsChanged.Broadcast(this);
	}
}

void AHeistDayPlayerState::AddDeath(int32 RoundNumber)
{
	if (HasAuthority())
	{
		if (RoundNumber == 1)
			FirstRound.DeathsCount++;
		else if (RoundNumber == 2)
			SecondRound.DeathsCount++;

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddDeath to player %d for Round %d. Total deaths: %d"), GetPlayerId(), RoundNumber, (RoundNumber == 1) ? FirstRound.DeathsCount : SecondRound.DeathsCount);

		OnStatsChanged.Broadcast(this);
	}
}

#pragma endregion

#pragma region State Setters

void AHeistDayPlayerState::SetCurrentHealth(int32 NewHealth)
{
	if (HasAuthority())
	{
		int32 OldHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp(NewHealth, int32(0), MaxHealth);

		// Trigger HUD damage event ONLY if health was lost
		if (CurrentHealth < OldHealth)
		{
			OnPlayerDamaged.Broadcast(this);
		}

		// Ensure death event is triggered only once (if not already dead)
		if (IsDead() && OldHealth > 0)
		{
			OnPlayerDied.Broadcast();
		}

		OnStatsChanged.Broadcast(this);

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] SetCurrentHealth = %d for PlayerId = %d"), CurrentHealth, GetPlayerId());
	}
}

void AHeistDayPlayerState::SetTeamId(int32 NewTeamId)
{
	if (HasAuthority())
	{
		TeamId = NewTeamId;
	}
}

void AHeistDayPlayerState::SetPlayerIndex(int32 NewPlayerIndex)
{
	if (HasAuthority())
	{
		this->PlayerIndex = NewPlayerIndex;
		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] SetPlayerIndex = %d"), PlayerIndex);
	}
}

void AHeistDayPlayerState::SetTeam(ETeam NewTeam)
{
	if (HasAuthority())
	{
		Team = NewTeam;
		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] SetTeam = %d"), (int32)Team);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerState] SetTeam called without authority!"));
	}
}

#pragma endregion

#pragma region Server RPCs

void AHeistDayPlayerState::Server_SetEpicName_Implementation(const FString& NewName)
{
	FString LocalName = NewName;

	if (LocalName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] EOS name empty, setting default"));
		LocalName = TEXT("Player");
	}

	EpicAccountName = LocalName;

	UE_LOG(LogTemp, Warning, TEXT("[Server] EOS name synchronized for PlayerState: %s"), *EpicAccountName);
}

void AHeistDayPlayerState::Server_ClientIsReady_Implementation()
{
	if (HasAuthority())
	{
		if (AHeistDayGameMode* GM = Cast<AHeistDayGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->OnClientReady();
		}
	}
}

#pragma endregion

#pragma region OnRep Callbacks

void AHeistDayPlayerState::OnRep_CurrentHealth(int32 OldHealth)
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] CurrentHealth received = %d (Old: %d) for PlayerId = %d"), CurrentHealth, OldHealth, GetPlayerId());

	if (CurrentHealth < OldHealth)
	{
		OnPlayerDamaged.Broadcast(this);
	}

	if (IsDead() && OldHealth > 0)
	{
		OnPlayerDied.Broadcast();
	}

	OnStatsChanged.Broadcast(this);
}

void AHeistDayPlayerState::OnRep_TeamId()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] TeamId received = %d for PlayerId = %d"), TeamId, GetPlayerId());
}

void AHeistDayPlayerState::OnRep_Team()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] Team received = %d for PlayerId = %d"), static_cast<int32>(Team), GetPlayerId());
}

void AHeistDayPlayerState::OnRep_PlayerIndex()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] PlayerIndex received = %d for PlayerId = %d"), PlayerIndex, GetPlayerId());
}

void AHeistDayPlayerState::OnRep_StatsChanged()
{
	OnStatsChanged.Broadcast(this);
}

#pragma endregion