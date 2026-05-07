#include "HeistDayPlayerState.h"
#include "Net/UnrealNetwork.h"

void AHeistDayPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHeistDayPlayerState, TeamId);
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
    }
}

void AHeistDayPlayerState::OnRep_TeamId()
{
    UE_LOG(LogTemp, Warning, TEXT("[Client] TeamId received = %d"), TeamId);
}

void AHeistDayPlayerState::OnRep_Team()
{
    UE_LOG(LogTemp, Warning, TEXT("[Client] Team received = %d"), static_cast<int32>(Team));
}

