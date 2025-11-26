// Include necessary Unreal headers
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Delegates/DelegateCombinations.h"
#include "CPP_GM_BottomlessPit.generated.h"

USTRUCT(BlueprintType)
struct FComboMultiplierStep
{
    GENERATED_BODY()

    // Minimal combo value required to reach this multiplier
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ComboThreshold = 0.0f;

    // Multiplier to use when threshold is reached
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Multiplier = 1.0f;
};

// Declare your delegate outside the class declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreUpdated, float, NewScore);

/**
 * GameMode class for BottomlessPit
 */
UCLASS()
class BOTTOMLESSPIT_API ACPP_GM_BottomlessPit : public AGameModeBase
{
    GENERATED_BODY()

public:
    // Constructor
    ACPP_GM_BottomlessPit();

    // Called when the game starts
    virtual void BeginPlay() override;

    /** Getter for score */
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    float GetScore() const { return PlayerScore; }

    // Start scoring
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void StartScoring();

    // Stop scoring
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void StopScoring();

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void AddScore(float Amount);

    /** Delegate broadcast whenever score changes */
    UPROPERTY(BlueprintAssignable, Category = "Scoring")
    FOnScoreUpdated OnScoreUpdated;

    /** Internal function to update score */
    void UpdateScore(float Amount);


    // How far the player must fall before earning another chunk of score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float StepSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float ScoreUpdateInterval = 0.3;

    // How many points per step
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    int32 PointsPerStep;

    // Keep track of the "next target depth"
    float NextScoreThresholdZ;

    // Combo
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void AddCombo(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Combo")
    void StopComboManually();

protected:
    /** Timer handle for depth checking */
    FTimerHandle DepthCheckTimerHandle;

    /** Score multiplier (points per unit fallen) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float ScoreMultiplier;

    /** Current score */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float PlayerScore;

    /** The lowest Z value reached */
    float LowestZ;

    /** Reference to tracked player pawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    AActor* TrackedActorRef;

    /** Function called on timer */
    void CheckPlayerDepth();

    // ---- Combo System ----

    // How long a combo stays alive without new hits (seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    float ComboDuration = 2.0f;

    // How often the combo timer updates (seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    float ComboTickInterval = 0.05f;

    // Current combo value
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo")
    float ComboValue = 0.0f;

    // Remaining time until combo expires
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo")
    float ComboRemainingTime = 0.0f;

    // Data-driven thresholds: when ComboValue >= ComboThreshold, use Multiplier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    TArray<FComboMultiplierStep> ComboMultiplierSteps;

    // Multiplier used when no step is active (base value)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    float DefaultComboMultiplier = 1.0f;

    // Current active combo multiplier (based on ComboValue)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo")
    float CurrentComboMultiplier = 1.0f;

    // Timer for combo countdown (no Tick)
    FTimerHandle ComboTimerHandle;

    // Internal tick called by ComboTimerHandle
    void ComboTick();

    // Internal to reset combo to zero and fire end event
    void EndCombo();

    // Recompute multiplier from ComboValue and threshold table
    void UpdateComboMultiplier();


    // --- Blueprint events you can implement in BP_GM ---

    // Fired ONCE when combo starts (going from 0/inactive to >0)
    UFUNCTION(BlueprintImplementableEvent, Category = "Combo")
    void OnComboStarted();

    // Fired when combo ends (timer reached 0)
    UFUNCTION(BlueprintImplementableEvent, Category = "Combo")
    void OnComboEnded();

    // Fired every time combo value increases (AddCombo called with >0)
    UFUNCTION(BlueprintImplementableEvent, Category = "Combo")
    void OnComboValueChanged(float NewComboValue);

    // Fired every timer tick with remaining percentage [1..0]
    // 1 = full time, 0 = expired
    UFUNCTION(BlueprintImplementableEvent, Category = "Combo")
    void OnComboTimePercentChanged(float PercentRemaining);

    // Fired whenever the combo multiplier actually changes
    UFUNCTION(BlueprintImplementableEvent, Category = "Combo")
    void OnComboMultiplierChanged(float NewMultiplier);
};



