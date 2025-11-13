// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/CPP_CameraManager.h"
#include "ACPP_DownwellCamera.generated.h"

/**
 * 
 */
UCLASS()
class BOTTOMLESSPIT_API AACPP_DownwellCamera : public ACPP_CameraManager
{
	GENERATED_BODY()
	

public:
	AACPP_DownwellCamera();

	virtual void BeginPlay() override;
	virtual FVector ComputeDesiredLocation(float DeltaSeconds) const override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | DownWell")
	float SoftZoneHalfWidth = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | DownWell")
	float DownLead = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | DownWell")
	float AutoScrollSpeed = 320.f; // uu/sec; down is negative Z

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | DownWell")
	float WellCenterX = 0.f;

    UPROPERTY(EditAnywhere, Category = "Downwell|Follow", meta = (ClampMin = "0", ClampMax = "1"))
    float FollowDownLerp = 1.f;       // 1 = snap to desired down, <1 smooth

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable | CameraManager | DownWell")
	bool bLockYToZero = true;

	UFUNCTION(BlueprintCallable, Category = "Endless")
	void NotifyWorldZLoop(float DeltaZ);



protected:

	mutable float LastCameraZ = 0.f;
    
};



