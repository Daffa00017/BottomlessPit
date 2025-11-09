// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTT/BTT_MoveToLocation2D_Snap.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "PaperFlipbookComponent.h"

UBTT_MoveToLocation2D_Snap::UBTT_MoveToLocation2D_Snap()
{
	NodeName = TEXT("Move To Location (2D XZ + Snap)");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTT_MoveToLocation2D_Snap::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return EBTNodeResult::Failed;

	Pawn = AIC->GetPawn();
	if (!Pawn.IsValid()) return EBTNodeResult::Failed;

	TargetLoc = BB->GetValueAsVector(LocationKey.SelectedKeyName);
	StartTime = Pawn->GetWorld()->GetTimeSeconds();
	return EBTNodeResult::InProgress;
}

void UBTT_MoveToLocation2D_Snap::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float)
{
	if (!Pawn.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* P = Pawn.Get();

	// success radius (XZ only)
	if (Dist2D_XZ(P->GetActorLocation(), TargetLoc) <= AcceptRadius)
	{
		if (UFloatingPawnMovement* M = P->FindComponentByClass<UFloatingPawnMovement>())
			M->StopMovementImmediately();

		// arrived -> Idle
		SetStateIfEnemy(P, EAIMovementState::Idle);

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// drive horizontal movement and mark Walk
	MoveHorizontalToward(P, TargetLoc);
	SetStateIfEnemy(P, EAIMovementState::Walk);

	// optional ground snap
	if (bSnapToGround) SnapDown(P);

	// timeout -> fail + Idle
	if (P->GetWorld()->GetTimeSeconds() - StartTime > MaxTime)
	{
		SetStateIfEnemy(P, EAIMovementState::Idle);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBTT_MoveToLocation2D_Snap::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (Pawn.IsValid())
	{
		if (UFloatingPawnMovement* M = Pawn->FindComponentByClass<UFloatingPawnMovement>())
			M->StopMovementImmediately();

		// aborted -> Idle
		SetStateIfEnemy(Pawn.Get(), EAIMovementState::Idle);
	}
	return EBTNodeResult::Aborted;
}

float UBTT_MoveToLocation2D_Snap::Dist2D_XZ(const FVector& A, const FVector& B)
{
	FVector D = A - B;
	D.Y = 0.f; // ignore Y for side-scroller (XZ plane). If your game is XY, zero Z instead.
	return D.Size();
}

void UBTT_MoveToLocation2D_Snap::MoveHorizontalToward(APawn* P, const FVector& To)
{
	FVector D = To - P->GetActorLocation();
	D.Y = 0.f;

	// flip sprite to face direction
	Flip(P, D.X);

	// horizontal input only
	const float DirX = FMath::Sign(D.X);
	P->AddMovementInput(FVector(DirX, 0.f, 0.f), 1.f);
}

void UBTT_MoveToLocation2D_Snap::SnapDown(APawn* P)
{
	const FVector Loc = P->GetActorLocation();
	const FVector Start = Loc + FVector(0, 0, 20);
	const FVector End = Start + FVector(0, 0, -FMath::Max(1.f, GroundTraceDistance));

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Move2D_Snap), false, P);

	if (P->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundTraceChannel, Q))
	{
		float HalfHeight = 0.f;
		if (const UCapsuleComponent* Cap = P->FindComponentByClass<UCapsuleComponent>())
			HalfHeight = Cap->GetScaledCapsuleHalfHeight();

		FVector NewLoc = Loc;
		NewLoc.Z = Hit.ImpactPoint.Z + HalfHeight;
		P->SetActorLocation(NewLoc, /*bSweep=*/true);
	}
}

void UBTT_MoveToLocation2D_Snap::Flip(APawn* P, float DX)
{
	if (FMath::Abs(DX) < 1.f) return;

	if (UPaperFlipbookComponent* Sprite = P->FindComponentByClass<UPaperFlipbookComponent>())
	{
		const bool bRight = DX >= 0.f;
		FVector S = Sprite->GetRelativeScale3D();
		S.X = FMath::Abs(S.X) * (bRight ? 1.f : -1.f);
		Sprite->SetRelativeScale3D(S);
	}
}



