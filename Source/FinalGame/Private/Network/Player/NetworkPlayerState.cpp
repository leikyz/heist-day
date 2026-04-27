#include "Network/Player/NetworkPlayerState.h"
#include "Net/UnrealNetwork.h"

void ANetworkPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ANetworkPlayerState, Team);
}

void ANetworkPlayerState::OnRep_Team()
{
    OnTeamChanged.Broadcast();
}

void ANetworkPlayerState::SetTeam(ETeam NewTeam)
{
    if (Team != NewTeam)
    {
        Team = NewTeam;
        OnTeamChanged.Broadcast();
    }
}