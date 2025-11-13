// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/HitopManagerActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "HAL/PlatformTime.h"

AHitopManagerActor::AHitopManagerActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bTickEvenWhenPaused = true; // critical for very low dilations
    SetActorTickEnabled(true);
    SetActorHiddenInGame(true);
}

void AHitopManagerActor::BeginPlay()
{
    Super::BeginPlay();
}

void AHitopManagerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bActive)
    {
        ApplyGlobal(1.0f);
        ClearExemptions();
        bActive = false;
    }
    Super::EndPlay(EndPlayReason);
}

void AHitopManagerActor::EndEffectIfStampMatches(double Stamp)
{
    // Ignore if a newer hitstop started (StartRealTime changed)
    if (!FMath::IsNearlyEqual(Stamp, StartRealTime))
    {
        return;
    }

    ApplyGlobal(1.0f);
    ClearExemptions();
    bActive = false;
}

// ------------ Public API -----------------

void AHitopManagerActor::PlayHitstopSimple(float Duration, float MinDilation)
{
    const float Dur = ClampDur(Duration, DurationClampMax);
    const float Min = ClampMinDil(MinDilation, MinDilationFloor);
    PlayHitstopAdvanced(Dur, Min, DefaultReleaseCurve.Get(), DefaultIntensity, TArray<AActor*>{});
}

void AHitopManagerActor::PlayHitstopAdvanced(
    float Duration,
    float MinDilation,
    UCurveFloat* ReleaseCurve,
    float Intensity,
    const TArray<AActor*>& ExemptActors)
{
    const float Dur = ClampDur(Duration, DurationClampMax);
    const float Min = ClampMinDil(MinDilation / FMath::Max(Intensity, 0.001f), MinDilationFloor);

    for (AActor* A : ExemptActors)
    {
        if (IsValid(A)) { ExemptSet.Add(A); }
    }

    // Start/refresh effect (uses timer for restore)
    UCurveFloat* UseCurve = ReleaseCurve ? ReleaseCurve : DefaultReleaseCurve.Get();
    BeginEffect(Min, Dur, UseCurve);
}

// ------------ Tick -----------------

void AHitopManagerActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

// ------------ Internals -----------------

void AHitopManagerActor::BeginEffect(float NewMin, float NewDur, UCurveFloat* Curve)
{
    StartMinDil = NewMin;
    ActiveDuration = NewDur;
    if (Curve) { ActiveCurve = Curve; }
    else { ActiveCurve = DefaultReleaseCurve; }

    StartRealTime = FPlatformTime::Seconds();
    bActive = true;

    ApplyGlobal(StartMinDil);
    ApplyExemptions(StartMinDil);

    // Capture a unique stamp for THIS effect
    const double ThisEffectStamp = StartRealTime;

    // World timers run in dilated time; scale by MinDil to get real-time Duration
    if (UWorld* W = GetWorld())
    {
        FTimerDelegate EndDel;
        EndDel.BindUObject(this, &AHitopManagerActor::EndEffectIfStampMatches, ThisEffectStamp);
        FTimerHandle TempHandle;
        W->GetTimerManager().SetTimer(TempHandle, EndDel, ActiveDuration * StartMinDil, false);
    }
}

void AHitopManagerActor::ApplyGlobal(float Dilation)
{
    if (UWorld* W = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(W, FMath::Max(Dilation, 0.0001f));
    }
}

void AHitopManagerActor::ApplyExemptions(float GlobalDil)
{
    //float GlobalDil = GlobalDil;
    if (GlobalDil <= 0.f) { GlobalDil = 0.0001f; }
    const float NeededCustom = 1.0f / GlobalDil;

    for (auto It = ExemptSet.CreateIterator(); It; ++It)
    {
        if (AActor* A = It->Get())
        {
            if (!SavedCustom.Contains(*It))
            {
                SavedCustom.Add(*It, A->CustomTimeDilation);
            }
            A->CustomTimeDilation = NeededCustom;
        }
        else
        {
            It.RemoveCurrent();
        }
    }
}

void AHitopManagerActor::ClearExemptions()
{
    for (auto& Pair : SavedCustom)
    {
        if (AActor* A = Pair.Key.Get())
        {
            A->CustomTimeDilation = Pair.Value;
        }
    }
    SavedCustom.Empty();
    ExemptSet.Empty();
}

void AHitopManagerActor::EndEffectSafely()
{
    if (!bActive) return;

    const double Now = FPlatformTime::Seconds();
    // If another BeginEffect happened after this timer was set, respect the newer one.
    if ((Now - StartRealTime) + KINDA_SMALL_NUMBER < ActiveDuration)
    {
        return; // a newer hitstop is active; ignore this older timer
    }

    ApplyGlobal(1.0f);
    ClearExemptions();
    bActive = false;
}

float AHitopManagerActor::EvalCurve01(float T01) const
{
    if (ActiveCurve)
    {
        return FMath::Clamp(ActiveCurve->GetFloatValue(T01), 0.f, 1.f);
    }
    // default ease-out if no curve set
    return FMath::InterpEaseOut(0.f, 1.f, T01, 2.0f);
}

float AHitopManagerActor::ClampMinDil(float In, float Floor)
{
    return FMath::Clamp(In, Floor, 0.2f);
}

float AHitopManagerActor::ClampDur(float In, float Max)
{
    return FMath::Clamp(In, 0.01f, Max);
}

