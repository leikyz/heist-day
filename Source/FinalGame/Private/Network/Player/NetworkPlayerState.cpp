#include "Network/Player/NetworkPlayerState.h"
#include "Net/UnrealNetwork.h"


void ANetworkPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANetworkPlayerState, Team);
    DOREPLIFETIME(ANetworkPlayerState, Round1Score);
    DOREPLIFETIME(ANetworkPlayerState, Round2Score);
    DOREPLIFETIME(ANetworkPlayerState, Coins);
    DOREPLIFETIME(ANetworkPlayerState, OriginalLobbyId);
}


void ANetworkPlayerState::SetTeam(ETeam NewTeam)
{
    if (Team != NewTeam)
    {
        Team = NewTeam;
        OnTeamChanged.Broadcast();
    }
}

void ANetworkPlayerState::OnRep_Team()
{
    OnTeamChanged.Broadcast();
}


void ANetworkPlayerState::SetRoundScore(int32 RoundNumber, int32 NewScore)
{
    if (RoundNumber == 1) Round1Score = NewScore;
    else                  Round2Score = NewScore;
    OnScoreChanged.Broadcast();
}

void ANetworkPlayerState::OnRep_Score()
{
    OnScoreChanged.Broadcast();
}


void ANetworkPlayerState::AddCoins(int32 Delta)
{
    Coins = FMath::Max(0, Coins + Delta);
    // OnRep_Score will fire on clients automatically via DOREPLIFETIME
}


void ANetworkPlayerState::SetOriginalLobbyId(const FString& LobbyId)
{
    OriginalLobbyId = LobbyId;
}