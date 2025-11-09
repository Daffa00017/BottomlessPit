// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTT/BTT_JumpUp2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"

UBTT_JumpUp2D::UBTT_JumpUp2D()
{
	NodeName = TEXT("Jump Up (2D simple)");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTT_JumpUp2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return EBTNodeResult::Failed;

	APawn* P = AIC->GetPawn();
	if (!P) return EBTNodeResult::Failed;

	Pawn = P;
	StartTime = P->GetWorld()->GetTimeSeconds();
	StartZ = P->GetActorLocation().Z;

	// Determine height
	float H = DesiredHeight;
	if (H <= 0.f)
	{
		if (AActor* T = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName)))
		{
			H = FMath::Max(120.f, T->GetActorLocation().Z - StartZ + 60.f);
		}
		else H = 200.f;
	}

	// v0 = sqrt(2 g h)
	Vz = FMath::Sqrt(FMath::Max(0.f, 2.f * Gravity * H));
	Vz = FMath::Min(Vz, MaxJumpSpeed);

	return EBTNodeResult::InProgress;
}

void UBTT_JumpUp2D::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DT)
{
	if (!Pawn.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* P = Pawn.Get();

	// Integrate vertical motion
	FVector Loc = P->GetActorLocation();
	Loc.Z += Vz * DT;
	Vz -= Gravity * DT;
	P->SetActorLocation(Loc);

	// Land check: moving down + ground under feet
	if (Vz <= 0.f && IsGrounded(P))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (P->GetWorld()->GetTimeSeconds() - StartTime > MaxTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

bool UBTT_JumpUp2D::IsGrounded(APawn* P) const
{
	const FVector Loc = P->GetActorLocation();
	float HalfHeight = 40.f;
	if (const UCapsuleComponent* Cap = P->FindComponentByClass<UCapsuleComponent>())
		HalfHeight = Cap->GetScaledCapsuleHalfHeight();

	const FVector Start = Loc + FVector(0, 0, 10);
	const FVector End = Loc - FVector(0, 0, HalfHeight + 5.f);

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Jump2D_Ground), false, P);
	return P->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundTraceChannel, Q);
}



