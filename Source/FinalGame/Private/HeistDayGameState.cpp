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
}

void AHeistDayGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority()) return;
    if (RemainingTime <= 0.f) return;

    RemainingTime = FMath::Max(0.f, RemainingTime - DeltaSeconds);
}

void AHeistDayGameState::Server_SetRemainingTime(float Seconds)
{
    RemainingTime = Seconds;
}

void AHeistDayGameState::Server_SetMatchPhase(EMatchPhase NewPhase)
{
    MatchPhase = NewPhase;
}

void AHeistDayGameState::OnRep_MatchPhase()
{
    OnMatchPhaseChanged.Broadcast(MatchPhase);
}


void AHeistDayGameState::OnRep_RemainingTime()
{
    OnRemainingTimeChanged.Broadcast(RemainingTime);

}