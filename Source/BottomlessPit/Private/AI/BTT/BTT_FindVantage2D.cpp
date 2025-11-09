// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTT/BTT_FindVantage2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTT_FindVantage2D::UBTT_FindVantage2D() { NodeName = TEXT("Find Vantage (same floor)"); }

EBTNodeResult::Type UBTT_FindVantage2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8*)
{
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!BB || !AIC) return EBTNodeResult::Failed;

    APawn* Me = AIC->GetPawn();
    const AActor* T = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Me || !T) return EBTNodeResult::Failed;

    UWorld* W = Me->GetWorld();
    const FVector Origin = Me->GetActorLocation();

    // Optional: last vantage to avoid picking the same spot again
    const bool bUseLast = !LastVantageKey.SelectedKeyName.IsNone();
    const FVector LastVantage = bUseLast ? BB->GetValueAsVector(LastVantageKey.SelectedKeyName) : FVector::ZeroVector;

    // Collect all valid candidates, then pick random
    TArray<FVector> Candidates;
    Candidates.Reserve(FMath::CeilToInt((MaxDistance / Step)) * 2);

    // Decide search order
    const bool StartRight = bRandomizeStartSide ? FMath::RandBool() : true;
    const float Signs[2] = { StartRight ? +1.f : -1.f, StartRight ? -1.f : +1.f };

    for (float d = Step; d <= MaxDistance; d += Step)
    {
        for (int i = 0; i < 2; ++i)
        {
            const float sign = Signs[i];

            // Optional jitter so we don't always probe exact grid points
            const float J = (Jitter > 0.f) ? FMath::FRandRange(-Jitter, +Jitter) : 0.f;

            const FVector ProbeXZ = Origin + FVector(sign * d + J, 0.f, 0.f);

            FVector Ground;
            if (!GroundAt(W, ProbeXZ, Ground)) continue;
            if (FMath::Abs(Ground.Z - Origin.Z) > SameFloorTolerance) continue; // same platform only

            // Skip too-close candidates
            if (FMath::Abs(Ground.X - Origin.X) < MinDeltaXFromOrigin) continue;
            if (bUseLast && FVector::Dist2D(Ground, LastVantage) < MinDeltaXFromLast) continue;

            // LOS from eye height
            const FVector Eye = Ground + FVector(0, 0, EyeHeight);
            if (!LOSFrom(W, Eye, T)) continue;

            Candidates.Add(Ground);
        }
    }

    if (Candidates.Num() == 0)
    {
        return EBTNodeResult::Failed; // no vantage found
    }

    // Choose a random candidate
    const int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
    const FVector Chosen = Candidates[Index];

    BB->SetValueAsVector(OutVantageKey.SelectedKeyName, Chosen);
    if (bUseLast) BB->SetValueAsVector(LastVantageKey.SelectedKeyName, Chosen);

    return EBTNodeResult::Succeeded;
}

bool UBTT_FindVantage2D::GroundAt(UWorld* W, const FVector& FromXZ, FVector& OutGround) const
{
	const FVector Start = FromXZ + FVector(0, 0, GroundTrace * 0.5f);
	const FVector End = FromXZ - FVector(0, 0, GroundTrace);
	FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(Vantage_Ground), false);
	if (W->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Q))
	{
		OutGround = Hit.ImpactPoint; return true;
	}
	return false;
}

bool UBTT_FindVantage2D::LOSFrom(UWorld* W, const FVector& From, const AActor* Target) const
{
	const FVector To = Target->GetActorLocation();
	FVector Dir = To - From; Dir.Y = 0.f;
	const float Dist = Dir.Size(); if (Dist <= KINDA_SMALL_NUMBER) return true;
	FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(Vantage_LOS), false);
	if (!W->LineTraceSingleByChannel(Hit, From, From + Dir.GetSafeNormal() * (Dist + 1.f), TraceChannel, Q)) return true;
	return Hit.GetActor() == Target;
}



