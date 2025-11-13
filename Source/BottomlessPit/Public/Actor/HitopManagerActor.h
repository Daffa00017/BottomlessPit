// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Curves/CurveFloat.h"
#include "HitopManagerActor.generated.h"

UCLASS()
class BOTTOMLESSPIT_API AHitopManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
    AHitopManagerActor();

    // --- SIMPLE: just duration & min dilation (recommended default path)
    UFUNCTION(BlueprintCallable, Category = "Hitstop")
    void PlayHitstopSimple(float Duration, float MinDilation);

    // --- ADVANCED: override curve, intensity, exemptions
    UFUNCTION(BlueprintCallable, Category = "Hitstop")
    void PlayHitstopAdvanced(
        float Duration,
        float MinDilation,
        UCurveFloat* ReleaseCurve,
        float Intensity,
        const TArray<AActor*>& ExemptActors
    );

    // Optional defaults you can set in BP
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop|Defaults")
    TObjectPtr<UCurveFloat> DefaultReleaseCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop|Defaults", meta = (ClampMin = "0.5", ClampMax = "4.0"))
    float DefaultIntensity = 1.0f;

    // Clamp guards
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop|Limits", meta = (ClampMin = "0.005", ClampMax = "0.2"))
    float MinDilationFloor = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop|Limits", meta = (ClampMin = "0.01", ClampMax = "0.5"))
    float DurationClampMax = 0.20f;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void EndEffectIfStampMatches(double Stamp);

private:
    // State
    bool bActive = false;
    bool bHidden = true;
    double StartRealTime = 0.0;       // undilated seconds
    float  ActiveDuration = 0.0f;     // real secs
    float  StartMinDil = 1.0f;        // target min at start
    TObjectPtr<UCurveFloat> ActiveCurve = nullptr;

    // Stacking
    float PendingExtend = 0.0f;
    float PendingMinDil = 1.0f;

    // Exemptions
    TSet<TWeakObjectPtr<AActor>> ExemptSet;
    TMap<TWeakObjectPtr<AActor>, float> SavedCustom;

    // Impl
    void BeginEffect(float NewMin, float NewDur, UCurveFloat* Curve);
    void ApplyGlobal(float Dilation);
    void ApplyExemptions(float CurrentGlobalDil);
    void ClearExemptions();
    void EndEffectSafely();

    float EvalCurve01(float T01) const;
    static float ClampMinDil(float In, float Floor);
    static float ClampDur(float In, float Max);

};
