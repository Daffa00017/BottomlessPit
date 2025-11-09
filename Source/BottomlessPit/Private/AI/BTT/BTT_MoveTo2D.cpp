// Fill out your copyright notice in the Description page of Project Settings.

//BTT_MoveTo2D CPP
#include "AI/BTT/BTT_MoveTo2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "PaperFlipbookComponent.h" // only used if you want auto-flip
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UBTT_MoveTo2D::UBTT_MoveTo2D()
{
	NodeName = TEXT("MoveTo2D");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTT_MoveTo2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();

	if (!BB || !AIC) return EBTNodeResult::Failed;

	APawn* Pawn = AIC->GetPawn();
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Pawn || !Target) return EBTNodeResult::Failed;

	CachedPawn = Pawn;
	CachedTarget = Target;
	StartTime = Pawn->GetWorld()->GetTimeSeconds();

	// Make sure movement exists (UFloatingPawnMovement recommended)
	if (!Pawn->FindComponentByClass<UFloatingPawnMovement>())
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveTo2D_Snap: Pawn has no UFloatingPawnMovement; will still attempt AddMovementInput."));
	}

	return EBTNodeResult::InProgress;
}

void UBTT_MoveTo2D::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!CachedPawn.IsValid() || !CachedTarget.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* Pawn = CachedPawn.Get();
	const FVector From = Pawn->GetActorLocation();
	const FVector To = CachedTarget->GetActorLocation();

	// Check success radius (2D in XZ plane; ignore Y)
	if (Dist2D_XZ(From, To) <= AcceptRadius)
	{
		if (UFloatingPawnMovement* Move = Pawn->FindComponentByClass<UFloatingPawnMovement>())
		{
			Move->StopMovementImmediately();
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Horizontal move toward target (ignore Z intent)
	MoveHorizontalToward(Pawn, To);

	// Optional: keep on ground
	if (bSnapToGround) SnapDownToGround(Pawn);

	// Timeout safety
	const float Now = Pawn->GetWorld()->GetTimeSeconds();
	if (Now - StartTime > MaxTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBTT_MoveTo2D::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (CachedPawn.IsValid())
	{
		if (UFloatingPawnMovement* Move = CachedPawn->FindComponentByClass<UFloatingPawnMovement>())
		{
			Move->StopMovementImmediately();
		}
	}
	return EBTNodeResult::Aborted;
}

float UBTT_MoveTo2D::Dist2D_XZ(const FVector& A, const FVector& B)
{
	FVector D = A - B;
	D.Y = 0.f; // ignore Y (2D side-scroller on XZ)
	return D.Size();
}

void UBTT_MoveTo2D::MoveHorizontalToward(APawn* Pawn, const FVector& TargetLoc)
{
	const FVector From = Pawn->GetActorLocation();
	FVector Delta = TargetLoc - From;

	// Ignore vertical intent so pawn won't climb toward the target's Z
	Delta.Y = 0.f; // XZ plane, lock Y

	// Flip sprite if desired
	UpdateFlip(Pawn, Delta.X);

	// Feed the movement component with purely horizontal direction
	const FVector Dir = FVector(FMath::Sign(Delta.X), 0.f, 0.f);
	Pawn->AddMovementInput(Dir, 1.f);
}

void UBTT_MoveTo2D::SnapDownToGround(APawn* Pawn)
{
	const FVector Loc = Pawn->GetActorLocation();

	// Trace down from a little above the pawn to catch ground
	const float Up = 20.f;
	const FVector Start = Loc + FVector(0.f, 0.f, Up);
	const FVector End = Start + FVector(0.f, 0.f, -FMath::Max(1.f, GroundTraceDistance));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Move2D_SnapDown), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	FHitResult Hit;
	const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundTraceChannel, Params);

#if 0 // set to 1 to debug
	DrawDebugLine(Pawn->GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.05f, 0, 1.f);
	if (bHit) DrawDebugPoint(Pawn->GetWorld(), Hit.ImpactPoint, 6.f, FColor::Yellow, false, 0.05f);
#endif

	if (!bHit) return;

	float HalfHeight = 0.f;
	if (const UCapsuleComponent* Cap = Pawn->FindComponentByClass<UCapsuleComponent>())
	{
		HalfHeight = Cap->GetScaledCapsuleHalfHeight();
	}
	// Place pawn so capsule bottom rests slightly above the hit point
	const float NewZ = Hit.ImpactPoint.Z + HalfHeight + GroundOffset;
	if (!FMath::IsNearlyEqual(NewZ, Loc.Z, 0.1f))
	{
		FVector NewLoc = Loc;
		NewLoc.Z = NewZ;
		// After computing NewLoc
		FHitResult SweepHit;
		Pawn->SetActorLocation(NewLoc, /*bSweep=*/true, &SweepHit, ETeleportType::None);
		// Optional: only accept if ground is “walkable”
		// const float MinWalkableNormalZ = 0.5f; // ~60°
		// if (SweepHit.IsValidBlockingHit() && SweepHit.Normal.Z < MinWalkableNormalZ) { /* handle steep slope */ }
	}
}

void UBTT_MoveTo2D::UpdateFlip(APawn* Pawn, float DeltaX)
{
	if (!bFlipSpriteX) return;

	// Flip only if we have intent to move left/right
	if (FMath::Abs(DeltaX) < 1.f) return;

	if (UPaperFlipbookComponent* Sprite = Pawn->FindComponentByClass<UPaperFlipbookComponent>())
	{
		const bool bRight = (DeltaX >= 0.f);
		FVector S = Sprite->GetRelativeScale3D();
		S.X = FMath::Abs(S.X) * (bRight ? 1.f : -1.f);
		Sprite->SetRelativeScale3D(S);
	}
}



