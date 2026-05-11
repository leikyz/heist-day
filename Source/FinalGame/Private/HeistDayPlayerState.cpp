#include "HeistDayPlayerState.h"
#include "Net/UnrealNetwork.h"
#include <HeistDayGameMode.h>

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

void AHeistDayPlayerState::SetCurrentHealth(int32 NewHealth)
{
    if (HasAuthority())
    {
        CurrentHealth = FMath::Clamp(NewHealth, int32(0), MaxHealth);

        if (CurrentHealth <= 0)
            OnPlayerDied.Broadcast();

        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] SetCurrentHealth = %d for PlayerId = %d"), CurrentHealth, GetPlayerId());
    }
}

void AHeistDayPlayerState::Server_SetEpicName_Implementation(const FString& NewName)
{
    FString LocalName = NewName;

    if (LocalName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Server] EOS name empty, setting default"));
        LocalName = TEXT("Player");
    }

    EpicAccountName = LocalName;

    UE_LOG(LogTemp, Warning, TEXT("[Server] Nom EOS synchronisé pour le PlayerState : %s"), *EpicAccountName);
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

void AHeistDayPlayerState::OnRep_CurrentHealth()
{
    UE_LOG(LogTemp, Warning, TEXT("[Client] CurrentHealth received = %d for PlayerId = %d"), CurrentHealth, GetPlayerId());

    if (CurrentHealth <= 0)
        OnPlayerDied.Broadcast();
}
void AHeistDayPlayerState::OnRep_StatsChanged()
{
    OnStatsChanged.Broadcast(this);
}

void AHeistDayPlayerState::AddKill(int32 RoundNumber)
{
    if (HasAuthority())
    {
        if (RoundNumber == 1)
            FirstRound.KillsCount++;
        else if (RoundNumber == 2)
            SecondRound.KillsCount++;

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddKill for Round %d. Total kills: %d"), RoundNumber, (RoundNumber == 1) ? FirstRound.KillsCount : SecondRound.KillsCount);

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

		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] AddDeath for Round %d. Total deaths: %d"), RoundNumber, (RoundNumber == 1) ? FirstRound.DeathsCount : SecondRound.DeathsCount);

        OnStatsChanged.Broadcast(this);
    }
}