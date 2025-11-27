// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/CPP_GM_BottomlessPit.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ACPP_GM_BottomlessPit::ACPP_GM_BottomlessPit()
{
    ScoreMultiplier = 1.0f;
    PlayerScore = 0.0f;
    LowestZ = FLT_MAX;

    // ---- Combo defaults ----
    ComboDuration = 2.0f;        // tweak in BP if you want
    ComboTickInterval = 0.05f;   // ~20 times per second
    ComboValue = 0.0f;
    ComboRemainingTime = 0.0f;

    DefaultComboMultiplier = 1.0f;
    CurrentComboMultiplier = DefaultComboMultiplier;
}

void ACPP_GM_BottomlessPit::BeginPlay()
{
    Super::BeginPlay();
}

void ACPP_GM_BottomlessPit::StartScoring()
{
    if (!TrackedActorRef)
        TrackedActorRef = UGameplayStatics::GetPlayerPawn(this, 0);

    if (TrackedActorRef)
    {
        float StartZ = TrackedActorRef->GetActorLocation().Z;
        LowestZ = StartZ;

        // Initialize threshold (each step down from start)
        NextScoreThresholdZ = StartZ - StepSize;

        GetWorldTimerManager().SetTimer(
            DepthCheckTimerHandle,
            this,
            &ACPP_GM_BottomlessPit::CheckPlayerDepth,
            ScoreUpdateInterval,
            true
        );
    }
}

void ACPP_GM_BottomlessPit::StopScoring()
{
    GetWorldTimerManager().ClearTimer(DepthCheckTimerHandle);
}

void ACPP_GM_BottomlessPit::AddScore(float Amount)
{
    UpdateScore(Amount);
}

void ACPP_GM_BottomlessPit::CheckPlayerDepth()
{

    if (!TrackedActorRef) return;

    float CurrentZ = TrackedActorRef->GetActorLocation().Z;

    // If below threshold, award points
    while (CurrentZ <= NextScoreThresholdZ)
    {
        UpdateScore(PointsPerStep);

        // Move the threshold down for the next step
        NextScoreThresholdZ -= StepSize;
    }

    // Track lowest (not strictly needed anymore, but good reference)
    if (CurrentZ < LowestZ)
    {
        LowestZ = CurrentZ;
    }
}

void ACPP_GM_BottomlessPit::ComboTick()
{
    // Safety: if misconfigured, just end the combo
    if (ComboDuration <= 0.0f || ComboTickInterval <= 0.0f)
    {
        EndCombo();
        return;
    }

    // Count down by the tick interval
    ComboRemainingTime -= ComboTickInterval;
    if (ComboRemainingTime < 0.0f)
    {
        ComboRemainingTime = 0.0f;
    }

    // Percentage from 1 (full) to 0 (empty)
    const float Percent =
        FMath::Clamp(ComboRemainingTime / ComboDuration, 0.0f, 1.0f);

    OnComboTimePercentChanged(Percent);

    // When time is gone, combo ends
    if (ComboRemainingTime <= 0.0f)
    {
        EndCombo();
    }
}

void ACPP_GM_BottomlessPit::EndCombo()
{
    // Already ended? do nothing
    if (!GetWorldTimerManager().IsTimerActive(ComboTimerHandle) && ComboValue <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(ComboTimerHandle);

    ComboValue = 0.0f;
    ComboRemainingTime = 0.0f;

    // Reset multiplier as if combo is 0
    UpdateComboMultiplier(); // will go back to DefaultComboMultiplier

    // Make sure UI bar hits 0
    OnComboTimePercentChanged(0.0f);

    // Let BP hide combo UI etc.
    OnComboEnded();
}

void ACPP_GM_BottomlessPit::UpdateComboMultiplier()
{
    // Start from default
    float NewMultiplier = DefaultComboMultiplier;

    // We pick the multiplier with the highest threshold <= current combo
    float BestThreshold = -FLT_MAX;

    for (const FComboMultiplierStep& Step : ComboMultiplierSteps)
    {
        if (ComboValue >= Step.ComboThreshold && Step.ComboThreshold >= BestThreshold)
        {
            BestThreshold = Step.ComboThreshold;
            NewMultiplier = Step.Multiplier;
        }
    }

    if (!FMath::IsNearlyEqual(NewMultiplier, CurrentComboMultiplier))
    {
        CurrentComboMultiplier = NewMultiplier;
        OnComboMultiplierChanged(CurrentComboMultiplier);
    }
}

void ACPP_GM_BottomlessPit::UpdateScore(float Amount)
{
    if (Amount == 0.0f)
    {
        return;
    }

    // Combine your old base multiplier with the combo multiplier
    const float EffectiveMultiplier = ScoreMultiplier * CurrentComboMultiplier;
    const float FinalDelta = Amount * EffectiveMultiplier;

    PlayerScore += FinalDelta;

    //UE_LOG(LogTemp, Log, TEXT("Score updated: Base=%f Mult=%f ComboMult=%f FinalDelta=%f Total=%f"),
    //    Amount, ScoreMultiplier, CurrentComboMultiplier, FinalDelta, PlayerScore);

    OnScoreUpdated.Broadcast(PlayerScore);
}

void ACPP_GM_BottomlessPit::AddCombo(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    const bool bHadActiveCombo =
        (ComboValue > 0.0f) && GetWorldTimerManager().IsTimerActive(ComboTimerHandle);

    // Increase combo value
    ComboValue += Amount;
    OnComboValueChanged(ComboValue);

    // Recalculate multiplier based on new combo value
    UpdateComboMultiplier();

    // Refill remaining time
    ComboRemainingTime = ComboDuration;

    // First time starting a combo (or timer was dead)
    if (!bHadActiveCombo)
    {
        if (ComboDuration > 0.0f && ComboTickInterval > 0.0f)
        {
            GetWorldTimerManager().SetTimer(
                ComboTimerHandle,
                this,
                &ACPP_GM_BottomlessPit::ComboTick,
                ComboTickInterval,
                true
            );
        }

        // Combo just started
        OnComboStarted();
        OnComboTimePercentChanged(1.0f); // full bar at start
    }
    else
    {
        // Combo already running, we just reset the bar back to full
        OnComboTimePercentChanged(1.0f);
    }
}

void ACPP_GM_BottomlessPit::StopComboManually()
{
    EndCombo();
}




