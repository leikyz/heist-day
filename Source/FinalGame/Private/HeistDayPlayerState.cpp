#include "HeistDayPlayerState.h"
#include "Net/UnrealNetwork.h"

void AHeistDayPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHeistDayPlayerState, TeamId);
    DOREPLIFETIME(AHeistDayPlayerState, Team);
}

void AHeistDayPlayerState::SetTeamId(int32 NewTeamId)
{
    if (HasAuthority())
    {
        TeamId = NewTeamId;
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