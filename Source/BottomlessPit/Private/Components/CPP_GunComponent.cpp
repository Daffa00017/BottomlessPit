// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CPP_GunComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UCPP_GunComponent::UCPP_GunComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

/* ================== PUBLIC API ================== */

void UCPP_GunComponent::ApplyFireMode(const FFireModeProfile& Profile, FName ProjectileRowName)
{
	CurrentProfile = Profile;
	CurrentProjectileRowName = ProjectileRowName;
	SetRoundsPerMinute(CurrentProfile.RoundsPerMinute);
	// make next shot immediate
	LastShotTime = -1.0;
}

void UCPP_GunComponent::SetRoundsPerMinute(float NewRPM)
{
	RoundsPerMinute = FMath::Max(0.f, NewRPM);

	// if we’re in an RPM loop, reschedule to new cadence
	if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(FireTimer))
	{
		const float Interval = ShotInterval();
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);

		if (Interval > 0.f)
		{
			LastShotTime = GetWorld()->GetTimeSeconds();
			GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UCPP_GunComponent::FireAccordingToMode, Interval, true, Interval);
		}
	}
}

bool UCPP_GunComponent::TryConsumeAmmo_Implementation(int32 Cost)
{
	// Default: always allowed. In BP override, subtract ammo and return true/false.
	return true;
}

void UCPP_GunComponent::StartFire()
{
	if (CurrentProfile.Mode == EFireMode::Burst)
	{
		BeginBurst(); // tap or hold just starts one burst
		return;
	}

	const float Interval = ShotInterval();
	FireOnce(); // may call FireTrigger()

	if (Interval > 0.f)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		const float Since = (LastShotTime < 0.0) ? 0.f : float(Now - LastShotTime);
		const float Remaining = FMath::Max(0.f, Interval - Since);

		GetWorld()->GetTimerManager().SetTimer(
			FireTimer, this, &UCPP_GunComponent::FireTrigger, Interval, true, Remaining
		);
	}
}

void UCPP_GunComponent::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimer);

	if (bBurstActive)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimer);
		bBurstActive = false;
		BurstShotsRemaining = 0;
	}
}

void UCPP_GunComponent::FireOnce()
{
	// Burst: tap should start a burst immediately, ignoring RPM gate and charging ONCE
	if (CurrentProfile.Mode == EFireMode::Burst)
	{
		if (!bBurstActive)
		{
			BeginBurst();   // BeginBurst does the single ammo charge
		}
		return;
	}

	// Non-burst: RPM-gated trigger
	const float Interval = ShotInterval();
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastShotTime < 0.0 || Now - LastShotTime + KINDA_SMALL_NUMBER >= Interval)
	{
		FireTrigger();     // <- charges ammo once, then fires volley
		LastShotTime = Now;
	}
}

/* ================== INTERNAL MODE LOGIC ================== */

void UCPP_GunComponent::FireAccordingToMode()
{
	switch (CurrentProfile.Mode)
	{
	case EFireMode::Shotgun:
	{
		const int32 Pellets = FMath::Max(1, CurrentProfile.ProjectileAmount);
		FireVolley(Pellets);
		break;
	}
	case EFireMode::Burst:
	{
		// If player single-taps in burst mode, begin a burst
		if (!bBurstActive)
		{
			BeginBurst();
		}
		break;
	}
	case EFireMode::Auto:
	case EFireMode::Semi:
	case EFireMode::Sniper:
	default:
	{
		// supports “double-shot” autos via ProjectileAmount > 1
		FireVolley(FMath::Max(1, CurrentProfile.ProjectileAmount));
		break;
	}
	}
}

void UCPP_GunComponent::BeginBurst()
{
	// Single ammo charge for the whole burst
	const int32 Cost = FMath::Max(0, CurrentProfile.AmmoUsePerTrigger);
	if (Cost > 0 && !TryConsumeAmmo(Cost))
	{
		bBurstActive = false;
		BurstShotsRemaining = 0;
		return; // not enough ammo, abort burst
	}

	bBurstActive = true;
	BurstShotsRemaining = FMath::Max(1, CurrentProfile.ProjectileAmount);

	// first shot now
	FireVolley(1);
	BurstShotsRemaining--;

	// schedule rest...
	const float Step = FMath::Max(0.f, CurrentProfile.BurstInterval);
	if (BurstShotsRemaining > 0 && Step > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(BurstTimer, this, &UCPP_GunComponent::BurstTick, Step, true, Step);
	}
	else
	{
		while (BurstShotsRemaining > 0)
		{
			FireVolley(1);
			BurstShotsRemaining--;
		}
		bBurstActive = false;
	}
}

void UCPP_GunComponent::BurstTick()
{
	if (BurstShotsRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimer);
		bBurstActive = false;
		return;
	}

	FireVolley(1);
	BurstShotsRemaining--;

	if (BurstShotsRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimer);
		bBurstActive = false;
	}
}

/* ================== SHOT CREATION ================== */

void UCPP_GunComponent::FireVolley(int32 Count)
{
	TArray<FVector> Dirs;
	GenerateShotDirs(Count, Dirs);

	// Call your BP pool interface via one event (no C++ spawn)
	for (const FVector& Dir : Dirs)
	{
		OnRequestSpawnShot(
			Dir,
			CurrentProjectileRowName,
			/*bPenetrate*/ CurrentProfile.bPenetrate,
			/*Scale*/ CurrentProfile.ProjectileScale,
			/*Damage*/ CurrentProfile.DamagePerProjectile,
			/*LifeOverride*/ CurrentProfile.ProjectileLifeSeconds
		);
	}
}

void UCPP_GunComponent::GenerateShotDirs(int32 Count, TArray<FVector>& OutDirs) const
{
	OutDirs.Reset();
	OutDirs.Reserve(Count);

	const AActor* Owner = GetOwner();
	const FVector Forward = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;

	if (Count <= 1)
	{
		OutDirs.Add(Forward);
		return;
	}

	// If spread ON, yaw around forward
	if (CurrentProfile.bUseSpread && CurrentProfile.SpreadAngleDeg > KINDA_SMALL_NUMBER)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			const float YawDelta = FMath::FRandRange(-CurrentProfile.SpreadAngleDeg, CurrentProfile.SpreadAngleDeg);
			const FRotator R(0.f, YawDelta, 0.f);
			OutDirs.Add(R.RotateVector(Forward).GetSafeNormal());
		}
	}
	else
	{
		// Spread OFF: still produce N identical directions (=> N pellets)
		for (int32 i = 0; i < Count; ++i)
		{
			OutDirs.Add(Forward);
		}
	}
}

void UCPP_GunComponent::FireTrigger()
{
	// FREE shots if cost <= 0
	const int32 Cost = FMath::Max(0, CurrentProfile.AmmoUsePerTrigger);

	// Charge ONCE per trigger for Auto/Semi/Sniper/Shotgun
	if (Cost > 0 && !TryConsumeAmmo(Cost))
	{
		// Out of ammo: stop any RPM loop
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}

	FireAccordingToMode(); // this will FireVolley() pellets or a single shot
}




