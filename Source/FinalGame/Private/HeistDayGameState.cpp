#include "HeistDayGameState.h"
#include "Net/UnrealNetwork.h"

AHeistDayGameState::AHeistDayGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    // We tick only on the server to count down RemainingTime.
    // Clients receive the replicated value directly.
    SetActorTickEnabled(true);
}

// Replication

void AHeistDayGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AHeistDayGameState, MatchPhase);
    DOREPLIFETIME(AHeistDayGameState, RemainingTime);
    DOREPLIFETIME(AHeistDayGameState, Round1Result);
    DOREPLIFETIME(AHeistDayGameState, Round2Result);
}

// Tick — server counts down RemainingTime 

void AHeistDayGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Only the server writes RemainingTime; clients just read the replicated copy.
    if (!HasAuthority()) return;

    if (MatchPhase == EMatchPhase::Round1 ||
        MatchPhase == EMatchPhase::BetweenRounds ||
        MatchPhase == EMatchPhase::Round2)
    {
        RemainingTime = FMath::Max(0.f, RemainingTime - DeltaSeconds);
        // Broadcast to local listeners on the server (clients get it via replication).
        OnRoundTimeUpdated.Broadcast(RemainingTime);
    }
}

// Server setters 

void AHeistDayGameState::Server_SetMatchPhase(EMatchPhase NewPhase)
{
    MatchPhase = NewPhase;
    // Also fire locally on the server so server-side listeners react.
    OnMatchPhaseChanged.Broadcast(NewPhase);
}

void AHeistDayGameState::Server_SetRemainingTime(float Seconds)
{
    RemainingTime = Seconds;
}

void AHeistDayGameState::Server_SetRound1Result(const FRoundResult& Result)
{
    Round1Result = Result;
}

void AHeistDayGameState::Server_SetRound2Result(const FRoundResult& Result)
{
    Round2Result = Result;
}

// Multicast RPCs

void AHeistDayGameState::Multicast_NotifyRoundStarted_Implementation(int32 RoundNumber)
{
    // Clients react to MatchPhase replication for the HUD switch.
    // This RPC is a reliable "ping" so Blueprint event graphs can play transitions.
    OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AHeistDayGameState::Multicast_NotifyRoundEnded_Implementation(const FRoundResult& Result)
{
    OnRoundEnded.Broadcast(Result);
}

void AHeistDayGameState::Multicast_NotifyMatchEnded_Implementation(const FRoundResult& R1, const FRoundResult& R2)
{
    OnMatchEnded.Broadcast(R1, R2);
}

// OnRep
void AHeistDayGameState::OnRep_RemainingTime()
{
    OnRoundTimeUpdated.Broadcast(RemainingTime);
}
void AHeistDayGameState::OnRep_MatchPhase()
{
    OnMatchPhaseChanged.Broadcast(MatchPhase);
}