// BTService Line Of Sight 2D CPP


#include "AI/BTService/BTS_LOS2D.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"

UBTS_LOS2D::UBTS_LOS2D()
{
	NodeName = TEXT("LOS (2D horizontal)");
	Interval = 0.15f; RandomDeviation = 0.05f; // 6–8 Hz
}

void UBTS_LOS2D::TickNode(UBehaviorTreeComponent& OwnerComp, uint8*, float)
{
	auto* BB = OwnerComp.GetBlackboardComponent();
	auto* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	const APawn* Me = AIC->GetPawn();
	const AActor* T = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Me || !T) { BB->SetValueAsBool(HasLOSKey.SelectedKeyName, false); return; }

	float Dist = 0.f;
	const bool bLOS = HasLOS_2D(Me, T, EyeHeight, SightRange, LOSChannel, Dist);
	BB->SetValueAsBool(HasLOSKey.SelectedKeyName, bLOS);
	if (DistKey.SelectedKeyName.IsNone() == false) BB->SetValueAsFloat(DistKey.SelectedKeyName, Dist);
}

bool UBTS_LOS2D::HasLOS_2D(const APawn* Me, const AActor* Target, float EyeZ, float Range, ECollisionChannel Chan, float& OutDist)
{
	const FVector M = Me->GetActorLocation();
	const FVector T = Target->GetActorLocation();

	FVector Dir = T - M; Dir.Y = 0.f;     // 2D XZ
	OutDist = Dir.Size(); if (OutDist > Range) return false;
	if (OutDist <= KINDA_SMALL_NUMBER) return true;

	const FVector Start = FVector(M.X, M.Y, M.Z + EyeZ);
	const FVector End = Start + Dir.GetSafeNormal() * (OutDist + 1.f);
	FHitResult Hit; FCollisionQueryParams P(SCENE_QUERY_STAT(LOS2D), false, Me);
	if (!Me->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, Chan, P)) return true;
	return Hit.GetActor() == Target;
}



