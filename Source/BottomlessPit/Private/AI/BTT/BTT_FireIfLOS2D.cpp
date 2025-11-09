//Fire if within Line Of Sight CPP


#include "AI/BTT/BTT_FireIfLOS2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "PaperFlipbookComponent.h"

UBTT_FireIfLOS2D::UBTT_FireIfLOS2D() { NodeName = TEXT("Fire If LOS (2D + cooldown)"); }

EBTNodeResult::Type UBTT_FireIfLOS2D::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8*)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return EBTNodeResult::Failed;

	APawn* Me = AIC->GetPawn();
	AActor* T = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	const bool bLOS = BB->GetValueAsBool(HasLOSKey.SelectedKeyName);
	if (!Me || !T || !bLOS) return EBTNodeResult::Failed;

	const float Now = Me->GetWorld()->GetTimeSeconds();
	const float Last = BB->GetValueAsFloat(LastShotTimeKey.SelectedKeyName);
	if (Now - Last < Cooldown) return EBTNodeResult::Failed;
	if (!LOS2D(Me, T, EyeHeight, AttackRange, LOSChannel)) return EBTNodeResult::Failed;

	if (bFaceTargetOnFire) FaceToward(Me, T);

	// Fire (BP event)
	if (UFunction* Fn = Me->FindFunction(TEXT("AI_FireOnce")))
	{
		Me->ProcessEvent(Fn, nullptr);
	}

	// Update cooldown time
	BB->SetValueAsFloat(LastShotTimeKey.SelectedKeyName, Now);

	// >>> NEW: request a fresh strafe on next MaintainDistance run
	if (!RequestNewStrafeKey.SelectedKeyName.IsNone())
	{
		BB->SetValueAsBool(RequestNewStrafeKey.SelectedKeyName, true);
	}

	return EBTNodeResult::Succeeded;
}

bool UBTT_FireIfLOS2D::LOS2D(const APawn* Me, const AActor* T, float EyeZ, float Range, ECollisionChannel Chan)
{
	const FVector M = Me->GetActorLocation();
	const FVector P = T->GetActorLocation();
	FVector D = P - M; D.Y = 0.f;  // 2D (XZ)
	const float Dist = D.Size(); if (Dist > Range) return false;

	const FVector S = FVector(M.X, M.Y, M.Z + EyeZ);
	const FVector E = S + D.GetSafeNormal() * (Dist + 1.f);
	FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(Fire_LOS), false, Me);
	if (!Me->GetWorld()->LineTraceSingleByChannel(Hit, S, E, Chan, Q)) return true;
	return Hit.GetActor() == T;
}

void UBTT_FireIfLOS2D::FaceToward(APawn* Me, const AActor* T) const
{
	const float DX = T->GetActorLocation().X - Me->GetActorLocation().X;
	if (FMath::Abs(DX) < 1.f) return; // deadzone

	if (bFlipSpriteX)
	{
		if (UPaperFlipbookComponent* Sprite = Me->FindComponentByClass<UPaperFlipbookComponent>())
		{
			const bool bRight = (DX >= 0.f);
			FVector S = Sprite->GetRelativeScale3D();
			S.X = FMath::Abs(S.X) * (bRight ? 1.f : -1.f);
			Sprite->SetRelativeScale3D(S);
		}
	}
	else
	{
		// alternative: 0/180 yaw (keeps positive scale)
		FRotator R = Me->GetActorRotation();
		R.Yaw = (DX >= 0.f) ? 0.f : 180.f;
		Me->SetActorRotation(R);
	}
}



