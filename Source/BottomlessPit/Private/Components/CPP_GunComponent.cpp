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


void UCPP_GunComponent::ApplyFireMode(const FFireModeProfile& Profile)
{
	CurrentProfile = Profile;
	CurrentProjectileRowName = Profile.ProjectileName;

	// stop any previous cadence or burst timers
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FireTimer);
		W->GetTimerManager().ClearTimer(BurstTimer);
	}

	bBurstActive = false;
	BurstShotsRemaining = 0;

	SetRoundsPerMinute(CurrentProfile.RoundsPerMinute);

	LastShotTime = -1.0;
	LastBurstTriggerTime = -1.0;    // <<< ADD THIS
	bWantsToFire = false;   // optional but nice to clean up
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
			GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UCPP_GunComponent::FireTrigger, Interval, true, Interval);
		}
	}
}

bool UCPP_GunComponent::TryConsumeAmmo_Implementation(int32 Cost)
{
	// Default: always allowed. In BP override, subtract ammo and return true/false.
	return true;
}

bool UCPP_GunComponent::CanBeginTrigger_Implementation()
{
	return true;
}

void UCPP_GunComponent::StartFire()
{
	bWantsToFire = true;

	if (CurrentProfile.Mode == EFireMode::Burst)
	{
		// One burst per press, internal RPM / ammo / gate handled in BeginBurst
		if (!bBurstActive)
		{
			BeginBurst();
		}
		return; // no RPM loop timer for burst
	}

	// ===== non-burst unchanged =====
	const float Interval = ShotInterval();
	FireOnce();
	if (Interval > 0.f)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		const float Since = (LastShotTime < 0.0) ? 0.f : float(Now - LastShotTime);
		const float Remain = FMath::Max(0.f, Interval - Since);

		GetWorld()->GetTimerManager().SetTimer(
			FireTimer, this, &UCPP_GunComponent::FireTrigger, Interval, true, Remain);
	}
}

void UCPP_GunComponent::StopFire()
{
	bWantsToFire = false;
    GetWorld()->GetTimerManager().ClearTimer(FireTimer);
    //GetWorld()->GetTimerManager().ClearTimer(AutoBurstTimer);
}

void UCPP_GunComponent::FireOnce()
{
	if (CurrentProfile.Mode == EFireMode::Burst)
	{
		if (!bBurstActive)
		{
			BeginBurst();
		}
		return;
	}

	// non-burst: RPM-gated single trigger
	const float  Interval = ShotInterval();
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastShotTime < 0.0 || Now - LastShotTime + KINDA_SMALL_NUMBER >= Interval)
	{
		FireTrigger();
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
		// Do nothing here. Bursts are started only from StartFire/FireOnce
		// after CanBeginTrigger() + IsBurstReady() checks.
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
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// ===== RPM gate per *burst* (not per bullet) =====
	const float  Interval = ShotInterval();              // 60 / RPM
	const double Now = World->GetTimeSeconds();

	if (Interval > 0.f &&
		LastBurstTriggerTime >= 0.0 &&
		(Now - LastBurstTriggerTime + KINDA_SMALL_NUMBER) < Interval)
	{
		// too soon for next burst, silently ignore this trigger
		return;
	}

	// same external gate as normal fire (ground check etc.)
	if (!CanBeginTrigger())
	{
		bBurstActive = false;
		BurstShotsRemaining = 0;
		return;
	}

	const int32 Cost = FMath::Max(0, CurrentProfile.AmmoUsePerTrigger);
	if (!TryConsumeAmmo(Cost))   // always ask BP, even when Cost == 0
	{
		bBurstActive = false;
		BurstShotsRemaining = 0;
		return;
	}

	// sync the clocks
	LastShotTime = Now;  // for FireOnce-based RPM feel, if you ever mix
	LastBurstTriggerTime = Now;  // this is what gates next burst

	OnTriggerFired(CurrentProfile);

	bBurstActive = true;
	BurstShotsRemaining = FMath::Max(1, CurrentProfile.ProjectileAmount);

	// first shot now
	FireVolley(1);
	--BurstShotsRemaining;

	const float Step = FMath::Max(0.f, CurrentProfile.BurstInterval);
	if (BurstShotsRemaining > 0 && Step > 0.f)
	{
		World->GetTimerManager().SetTimer(
			BurstTimer, this, &UCPP_GunComponent::BurstTick, Step, true, Step);
	}
	else
	{
		// either no interval or only 1 bullet: just dump the rest immediately
		while (BurstShotsRemaining > 0)
		{
			FireVolley(1);
			--BurstShotsRemaining;
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
	if (!CanBeginTrigger())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}

	const int32 Cost = FMath::Max(0, CurrentProfile.AmmoUsePerTrigger);
	if (!TryConsumeAmmo(Cost))
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}

	FireAccordingToMode();
	OnTriggerFired(CurrentProfile);
}

bool UCPP_GunComponent::IsBurstReady() const
{
	const float Interval = ShotInterval(); // 60 / RPM
	const double Now = GetWorld()->GetTimeSeconds();

	// Ready if we’ve never fired OR enough time since the last trigger (any mode)
	if (Interval <= 0.f) return true; // 0 RPM => let BP gate it; reads clearer in jam
	return (LastShotTime < 0.0) || (Now - LastShotTime + KINDA_SMALL_NUMBER >= Interval);
}




