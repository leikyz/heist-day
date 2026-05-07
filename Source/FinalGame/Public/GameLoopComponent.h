//#pragma once
//
//#include "CoreMinimal.h"
//#include "Components/ActorComponent.h"
//#include "HeistDayGameState.h"
//#include "GameLoopComponent.generated.h"
//
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerTickDelegate, float, RemainingSeconds);
//
//UCLASS(Blueprintable, ClassGroup = "Game Loop", meta = (BlueprintSpawnableComponent))
//class FINALGAME_API UGameLoopComponent : public UActorComponent
//{
//    GENERATED_BODY()
//
//public:
//    UGameLoopComponent();
//
//    virtual void BeginPlay() override;
//    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//
//    // ── Blueprint events — override these in your BP subclass ─────────────────
//
//    /** Called (on client) when a round starts. RoundNumber is 1 or 2. */
//    UFUNCTION(BlueprintImplementableEvent, Category = "Game Loop|Events")
//    void OnRoundStarted(int32 RoundNumber);
//
//    // Called every frame-ish while a timed phase is active. (Event Dispatcher pour les Widgets)
//    UPROPERTY(BlueprintAssignable, Category = "Heist|Events")
//    FOnTimerTickDelegate OnTimerTickEvent;
//
//    // Called when a round ends.
//    UFUNCTION(BlueprintImplementableEvent, Category = "Game Loop|Events")
//    void OnRoundEnded(const FRoundResult& Result);
//
//    // Called when both rounds are complete.
//    UFUNCTION(BlueprintImplementableEvent, Category = "Game Loop|Events")
//    void OnMatchEnded(const FRoundResult& Round1Result, const FRoundResult& Round2Result);
//
//    // Called when the match phase changes.
//    UFUNCTION(BlueprintImplementableEvent, Category = "Game Loop|Events")
//    void OnPhaseChanged(EMatchPhase NewPhase);
//
//    // ── Blueprint callable helpers ────────────────────────────────────────────
//
//    UFUNCTION(BlueprintPure, Category = "Game Loop")
//    EMatchPhase GetCurrentPhase() const;
//
//    UFUNCTION(BlueprintPure, Category = "Game Loop")
//    float GetRemainingTime() const;
//
//private:
//    void BindToGameState();
//
//    // Delegate handlers
//    UFUNCTION()
//    void HandlePhaseChanged(EMatchPhase NewPhase);
//
//    UFUNCTION()
//    void HandleTimerTick(float RemainingSeconds);
//
//    UFUNCTION()
//    void HandleRoundEnded(const FRoundResult& Result);
//
//    UFUNCTION()
//    void HandleMatchEnded(const FRoundResult& R1, const FRoundResult& R2);
//
//    TWeakObjectPtr<AHeistDayGameState> CachedGameState;
//
//    FTimerHandle RetryBindHandle;
//};