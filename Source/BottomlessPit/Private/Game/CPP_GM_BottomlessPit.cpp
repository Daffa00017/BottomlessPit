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
    PlayerScore = PlayerScore+Amount;
    OnScoreUpdated.Broadcast(PlayerScore);
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

void ACPP_GM_BottomlessPit::UpdateScore(float Amount)
{
    PlayerScore += Amount;

    // Log for debug (replace with delegate/UI hook)
    //UE_LOG(LogTemp, Log, TEXT("Score updated: %f"), PlayerScore);

    OnScoreUpdated.Broadcast(PlayerScore);
}




