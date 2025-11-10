// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CPP_GunComponent.h"
#include "Actor/CPP_Gun.h"

// Sets default values for this component's properties
UCPP_GunComponent::UCPP_GunComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UCPP_GunComponent::StartFire()
{
	const float Interval = ShotInterval();

	// Try one shot now (RPM-gated). We don't need the return value here.
	FireOnce();

	if (Interval > 0.f)
	{
		// Align the first tick with the remaining cooldown
		const double Now = GetWorld()->GetTimeSeconds();
		const float Since = (LastShotTime < 0.0) ? 0.f : float(Now - LastShotTime);
		const float Remaining = FMath::Max(0.f, Interval - Since);

		GetWorld()->GetTimerManager().SetTimer(
			FireTimer, this, &UCPP_GunComponent::FireTick, Interval, true, Remaining
		);
	}
}

void UCPP_GunComponent::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimer);
}

void UCPP_GunComponent::FireOnce()
{
	const float Interval = ShotInterval();
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastShotTime < 0.0 || Now - LastShotTime + KINDA_SMALL_NUMBER >= Interval)
	{
		FireEvent();            // do the actual shot (your gun hitscan)
		LastShotTime = Now;
	}
	// else: still on cooldown to respect RPM
}

void UCPP_GunComponent::FireEvent_Implementation()
{
}

// Called when the game starts
void UCPP_GunComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UCPP_GunComponent::FireTick()
{
	FireEvent();                                 // calls BP override if present
	LastShotTime = GetWorld()->GetTimeSeconds(); // keep semi-auto in sync, too
}


// Called every frame
void UCPP_GunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}




