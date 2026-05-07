//#include "GameLoopComponent.h"
//#include "Kismet/GameplayStatics.h"
//#include "Engine/World.h"
//
//UGameLoopComponent::UGameLoopComponent()
//{
//    PrimaryComponentTick.bCanEverTick = false;
//    SetIsReplicatedByDefault(false); // Pure client-side driver.
//}
//
//void UGameLoopComponent::BeginPlay()
//{
//    Super::BeginPlay();
//
//    // Only run on the local owning client (or in standalone / PIE).
//    // On the dedicated server this component has no owner PC to bind from.
//    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
//    {
//        BindToGameState();
//    }
//    else if (GetWorld() && !GetWorld()->IsNetMode(NM_DedicatedServer))
//    {
//        // Standalone or listen-server local player
//        BindToGameState();
//    }
//}
//
//void UGameLoopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
//{
//    GetWorld()->GetTimerManager().ClearTimer(RetryBindHandle);
//
//    if (CachedGameState.IsValid())
//    {
//        CachedGameState->OnMatchPhaseChanged.RemoveDynamic(this, &UGameLoopComponent::HandlePhaseChanged);
//        CachedGameState->OnRoundTimeUpdated.RemoveDynamic(this, &UGameLoopComponent::HandleTimerTick);
//        CachedGameState->OnRoundEnded.RemoveDynamic(this, &UGameLoopComponent::HandleRoundEnded);
//        CachedGameState->OnMatchEnded.RemoveDynamic(this, &UGameLoopComponent::HandleMatchEnded);
//    }
//
//    Super::EndPlay(EndPlayReason);
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
//// BindToGameState — retries until the replicated GS is available
//// ─────────────────────────────────────────────────────────────────────────────
//void UGameLoopComponent::BindToGameState()
//{
//    AHeistDayGameState* GS = GetWorld()
//        ? Cast<AHeistDayGameState>(GetWorld()->GetGameState())
//        : nullptr;
//
//    if (!GS)
//    {
//        // GameState not replicated yet — retry in 0.5 s.
//        GetWorld()->GetTimerManager().SetTimer(RetryBindHandle, [this]()
//            {
//                BindToGameState();
//            }, 0.5f, false);
//        return;
//    }
//
//    CachedGameState = GS;
//
//    GS->OnMatchPhaseChanged.AddDynamic(this, &UGameLoopComponent::HandlePhaseChanged);
//    GS->OnRoundTimeUpdated.AddDynamic(this, &UGameLoopComponent::HandleTimerTick);
//    GS->OnRoundEnded.AddDynamic(this, &UGameLoopComponent::HandleRoundEnded);
//    GS->OnMatchEnded.AddDynamic(this, &UGameLoopComponent::HandleMatchEnded);
//
//    UE_LOG(LogTemp, Log, TEXT("[GameLoopComponent] Bound to AHeistDayGameState."));
//
//    // If the client joined mid-phase (e.g. reconnect), fire the current phase immediately.
//    HandlePhaseChanged(GS->GetMatchPhase());
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
//// Delegate handlers → fire Blueprint implementable events
//// ─────────────────────────────────────────────────────────────────────────────
//void UGameLoopComponent::HandlePhaseChanged(EMatchPhase NewPhase)
//{
//    OnPhaseChanged(NewPhase);
//
//    if (NewPhase == EMatchPhase::Round1)  OnRoundStarted(1);
//    if (NewPhase == EMatchPhase::Round2)  OnRoundStarted(2);
//}
//
//void UGameLoopComponent::HandleTimerTick(float RemainingSeconds)
//{
//    OnTimerTickEvent.Broadcast(RemainingSeconds);
//}
//
//void UGameLoopComponent::HandleRoundEnded(const FRoundResult& Result)
//{
//    OnRoundEnded(Result);
//}
//
//void UGameLoopComponent::HandleMatchEnded(const FRoundResult& R1, const FRoundResult& R2)
//{
//    OnMatchEnded(R1, R2);
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
//// Helpers
//// ─────────────────────────────────────────────────────────────────────────────
//EMatchPhase UGameLoopComponent::GetCurrentPhase() const
//{
//    return CachedGameState.IsValid()
//        ? CachedGameState->GetMatchPhase()
//        : EMatchPhase::WaitingForPlayers;
//}
//
//float UGameLoopComponent::GetRemainingTime() const
//{
//    return CachedGameState.IsValid() ? CachedGameState->GetRemainingTime() : 0.f;
//}