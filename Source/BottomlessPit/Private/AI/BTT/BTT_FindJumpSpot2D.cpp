// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTT/BTT_FindJumpSpot2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

UBTT_FindJumpSpot2D::UBTT_FindJumpSpot2D()
{
	NodeName = TEXT("FindJumpSpot2D");
}

EBTNodeResult::Type UBTT_FindJumpSpot2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return EBTNodeResult::Failed;

	APawn* Me = AIC->GetPawn();
	if (!Me) return EBTNodeResult::Failed;

	UWorld* W = Me->GetWorld();
	if (!W) return EBTNodeResult::Failed;

	const FVector Origin = Me->GetActorLocation();

	// Estimate needed headroom from target height difference
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	float NeedHeadroom = RequiredHeadroom;
	if (Target)
	{
		const float dz = Target->GetActorLocation().Z - Origin.Z;
		NeedHeadroom = FMath::Max(RequiredHeadroom, dz + 50.f); // enough to reach target ledge
	}

	// Try right, then left
	FVector Spot;
	const bool bRight = FindCandidate(W, Origin, +1.f, Spot, NeedHeadroom);
	const bool bLeft = bRight ? false : FindCandidate(W, Origin, -1.f, Spot, NeedHeadroom);

	if (bRight || bLeft)
	{
		BB->SetValueAsVector(JumpSpotKey.SelectedKeyName, Spot);
		BB->SetValueAsBool(HasJumpSpotKey.SelectedKeyName, true);
		return EBTNodeResult::Succeeded;
	}
	BB->SetValueAsBool(HasJumpSpotKey.SelectedKeyName, false);
	return EBTNodeResult::Failed;
}

bool UBTT_FindJumpSpot2D::FindCandidate(UWorld* W, const FVector& Origin, const float DirSign, FVector& OutSpot, float NeedHeadroomZ) const
{
	for (float d = Step; d <= MaxSearchDistance; d += Step)
	{
		FVector Test = Origin + FVector(DirSign * d, 0.f, 0.f);

		FVector Ground;
		if (!GroundAt(W, Test, Ground)) continue;

		if (HeadroomClear(W, Ground, NeedHeadroomZ))
		{
			OutSpot = Ground;
			return true;
		}
	}
	return false;
}

bool UBTT_FindJumpSpot2D::GroundAt(UWorld* W, const FVector& From, FVector& OutGround) const
{
	const FVector Start = From + FVector(0, 0, 50);
	const FVector End = Start + FVector(0, 0, -GroundTrace);

	FCollisionQueryParams P(SCENE_QUERY_STAT(JumpSpot_Ground), false);
	FHitResult Hit;
	if (W->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, P))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

bool UBTT_FindJumpSpot2D::HeadroomClear(UWorld* W, const FVector& GroundLoc, float Height) const
{
	// Capsule size guess (if your pawn has a capsule, read it)
	float Radius = 18.f, HalfHeight = 44.f;
	// We use a simple vertical capsule sweep to check ceiling clearance
	const FVector Start = GroundLoc + FVector(0, 0, HalfHeight + TopMargin);
	const FVector End = Start + FVector(0, 0, Height);

	FCollisionQueryParams P(SCENE_QUERY_STAT(JumpSpot_Headroom), false);
	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	// true if nothing blocks upward sweep
	return !W->SweepTestByChannel(Start, End, FQuat::Identity, TraceChannel, Shape, P);
}



