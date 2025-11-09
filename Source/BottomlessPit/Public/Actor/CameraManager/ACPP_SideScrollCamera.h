// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/CPP_CameraManager.h"
#include "ACPP_SideScrollCamera.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API AACPP_SideScrollCamera : public ACPP_CameraManager
{
	GENERATED_BODY()
	
public:
	// soft horizontal follow
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | SideScroller")
	float SoftZoneHalfWidth = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | SideScroller")
	bool bLockYToZero = true;

protected:
	virtual FVector ComputeDesiredLocation(float DeltaSeconds) const override
	{
		FVector Desired = GetActorLocation();
		if (FollowTarget)
		{
			const FVector T = FollowTarget->GetActorLocation();
			const float dx = T.X - Desired.X;
			if (FMath::Abs(dx) > SoftZoneHalfWidth)
			{
				Desired.X += (dx - FMath::Sign(dx) * SoftZoneHalfWidth);
			}
			// Optional: vertical ease to target.Z
			// Desired.Z = FMath::FInterpTo(Desired.Z, T.Z, DeltaSeconds, 1.5f);
		}
		if (bLockYToZero) Desired.Y = 0.f;
		return Desired;
	}
};



