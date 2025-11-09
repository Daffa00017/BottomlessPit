// CPP_ProjectileParent.cpp


#include "Actor/CPP_ProjectileParent.h"

#include "Components/SphereComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h" 
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/CollisionProfile.h"



ACPP_ProjectileParent::ACPP_ProjectileParent()
{
	PrimaryActorTick.bCanEverTick = true; // Only used if Stats.bUseFixedStep == false

	// Root: small query sphere (cheap, no physics)
	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	SetRootComponent(HitSphere);
	HitSphere->InitSphereRadius(Stats.CollisionRadius);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HitSphere->SetGenerateOverlapEvents(true);

	// Default responses:
	// - Block WorldStatic / WorldDynamic so swept move catches geometry.
	// - Overlap Pawn by default (tweak in BP/child as needed).
	HitSphere->SetCollisionResponseToAllChannels(ECR_Block);

	// Visual flipbook component
	Visual = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Visual"));
	Visual->SetupAttachment(HitSphere);
	Visual->SetLooping(false);
	Visual->bAutoActivate = false;

	// Start inactive; FireFrom will arm
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ACPP_ProjectileParent::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap + flipbook finished
	HitSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPP_ProjectileParent::OnHitSphereBeginOverlap);
	Visual->OnFinishedPlaying.AddDynamic(this, &ACPP_ProjectileParent::OnFlipbookFinished);

	// Ensure radius matches current Stats value
	HitSphere->SetSphereRadius(Stats.CollisionRadius);
}

void ACPP_ProjectileParent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If we’re in fixed-step mode, movement is timer-driven (no per-frame stepping here)
	if (Stats.bUseFixedStep) return;

	StepMove(DeltaSeconds);
}

// --------------------------- Config & Fire ---------------------------

void ACPP_ProjectileParent::ApplyRuntimeConfig(const FProjectileAnimResolved& InConfig)
{
	// Flipbooks (prefer loaded ones if provided)
	if (InConfig.SpawnFlipbook)  SpawnFlipbook = InConfig.SpawnFlipbook;
	if (InConfig.LoopFlipbook)   LoopFlipbook = InConfig.LoopFlipbook;
	if (InConfig.ImpactFlipbook) ImpactFlipbook = InConfig.ImpactFlipbook;

	// Copy full stat bundle
	Stats = InConfig.Stats;

	// Apply sphere size immediately
	HitSphere->SetSphereRadius(Stats.CollisionRadius);
}

void ACPP_ProjectileParent::FireFrom(const FVector& StartLocation, AActor* InstigatorActor)
{
	// Reset runtime state
	bActive = false;
	bHasImpacted = false;
	TraveledDistance = 0.f;
	DownVelocity = FVector(0.f, 0.f, -FMath::Max(0.f, Stats.Speed));
	LastImpactPoint = FVector::ZeroVector;
	LastImpactNormal = FVector::UpVector;
	FlipState = EProjectileFlipbookState::None;

	// Instigator for attribution (you can use in your BP logic)
	if (APawn* AsPawn = Cast<APawn>(InstigatorActor))
	{
		SetInstigator(AsPawn);
	}
	if (InstigatorActor)
	{
		SetOwner(InstigatorActor);
	}

	// Teleport to start
	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// Ensure Tick matches stepping mode
	PrimaryActorTick.bCanEverTick = !Stats.bUseFixedStep;

	// Arm projectile
	ArmAndGo();
}

void ACPP_ProjectileParent::FireFromBP(const FVector& StartLocation)
{
	AActor* UseInst = GetOwner();
	if (!UseInst) UseInst = GetInstigator();
	FireFrom(StartLocation, UseInst);
}

// --------------------------- Movement & Timers ---------------------------

void ACPP_ProjectileParent::ArmAndGo()
{
	// Enable interaction/visibility
	bActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Flipbook flow: Spawn (if set) -> auto Loop, else straight Loop
	if (SpawnFlipbook) SetFlipbookState(EProjectileFlipbookState::Spawn);
	else               SetFlipbookState(EProjectileFlipbookState::Loop);

	// Lifetime -> deactivate (pool return), not destroy
	if (Stats.MaxLifeSeconds > 0.f)
	{
		GetWorldTimerManager().ClearTimer(LifeTimerHandle);
		GetWorldTimerManager().SetTimer(
			LifeTimerHandle,
			this,
			&ACPP_ProjectileParent::Deactivate_Internal,
			Stats.MaxLifeSeconds,
			false
		);
	}

	// Movement driver
	if (Stats.bUseFixedStep)
	{
		StartFixedStep();
	}
}

void ACPP_ProjectileParent::Disarm()
{
	StopFixedStep();
	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	// Stop visuals & disable collision
	if (Visual) Visual->Stop();
	SetActorEnableCollision(false);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);

	bActive = false;
}

void ACPP_ProjectileParent::Deactivate_Internal()
{
	// Central “return to pool” path
	Disarm();

	// Reset transient state for next reuse
	bHasImpacted = false;
	TraveledDistance = 0.f;
	DownVelocity = FVector(0.f, 0.f, -FMath::Max(0.f, Stats.Speed));
	LastImpactPoint = FVector::ZeroVector;
	LastImpactNormal = FVector::UpVector;
	FlipState = EProjectileFlipbookState::None;

	// Notify pool manager (GameMode/etc.)
	OnDeactivated.Broadcast(this);

	// NOTE: Do NOT Destroy(). Your pool keeps a reference and will re-activate later.
}

void ACPP_ProjectileParent::HandleBlockingHit(const FHitResult& Hit)
{
}

void ACPP_ProjectileParent::DeactivateAndReturnToPool()
{
	Deactivate_Internal();
}

void ACPP_ProjectileParent::StartFixedStep()
{
	StopFixedStep();

	const float ClampedHz = FMath::Clamp(Stats.FixedStepHz, 30.f, 480.f);
	const float StepDt = 1.f / ClampedHz;

	GetWorldTimerManager().SetTimer(
		FixedStepTimerHandle,
		[this, StepDt]()
		{
			StepMove(StepDt);
		},
		StepDt,
		true
	);
}

void ACPP_ProjectileParent::StopFixedStep()
{
	if (FixedStepTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(FixedStepTimerHandle);
	}
}

void ACPP_ProjectileParent::StepMove(float Dt)
{
	if (!bActive || bHasImpacted || Dt <= 0.f || Stats.Speed <= 0.f || !RootComponent)
		return;

	const FVector Delta = DownVelocity * Dt; // (0,0,-Speed) * dt

	FHitResult Hit;
	const bool bMoved = RootComponent->MoveComponent(
		Delta, GetActorRotation(),
		/*bSweep=*/true, &Hit,
		MOVECOMP_NoFlags, ETeleportType::None);

	if (bMoved && Hit.bBlockingHit)
	{
		// Broadcast blocking contact; BP decides what to do.
		OnBlock.Broadcast(this, Hit);

		if (bAutoImpactOnHit)
		{
			TriggerImpactAndDeactivate(Hit);
		}
		return;
	}

	// Distance guard
	TraveledDistance += FMath::Abs(Delta.Z); // Z is negative; track magnitude
	if (Stats.MaxTravelDistance > 0.f && TraveledDistance >= Stats.MaxTravelDistance)
	{
		Deactivate_Internal();
	}
}

// --------------------------- Collision / Overlap ---------------------------

void ACPP_ProjectileParent::OnImpactAnimFinishTimer()
{
	// If we’re still in Impact (and not already deactivated), finish up.
	if (FlipState == EProjectileFlipbookState::Impact && bActive)
	{
		Deactivate_Internal(); // <- will broadcast OnDeactivated(this)
	}
}

void ACPP_ProjectileParent::OnHitSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& SweepResult)
{
	if (!bActive || bHasImpacted || !IsValid(OtherActor) || OtherActor == this)
		return;

	// Ignore self/owner/instigator (player)
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
		return;

	// Convert this “soft” contact into a synthetic blocking hit so the behavior matches StepMove()
	FHitResult AsBlock = SweepResult;
	AsBlock.bBlockingHit = true;

	// Choose a reasonable impact point/normal if SweepResult was thin
	if (!AsBlock.ImpactPoint.IsNearlyZero())
	{
		AsBlock.ImpactPoint = FVector(AsBlock.ImpactPoint); // cast from NetQuantize to FVector
	}
	else
	{
		AsBlock.ImpactPoint = GetActorLocation();
	}
	if (AsBlock.ImpactNormal.IsNearlyZero())
	{
		AsBlock.ImpactNormal = FVector::UpVector;
	}

	// Single path: everything is treated as “blocked”
	OnBlock.Broadcast(this, AsBlock);

	if (bAutoImpactOnHit)
	{
		TriggerImpactAndDeactivate(AsBlock);
	}
}

// --------------------------- Flipbook FSM & Impact ---------------------------

void ACPP_ProjectileParent::SetFlipbookState(EProjectileFlipbookState NewState,
	const FVector& /*ImpactPoint*/, const FVector& /*ImpactNormal*/)
{
	FlipState = NewState;

	switch (NewState)
	{
	case EProjectileFlipbookState::Spawn:
		Visual->SetFlipbook(SpawnFlipbook);
		Visual->SetLooping(false);
		Visual->PlayFromStart();
		break;

	case EProjectileFlipbookState::Loop:
		Visual->SetFlipbook(LoopFlipbook);
		Visual->SetLooping(true);
		Visual->Play();
		break;

	case EProjectileFlipbookState::Impact:
		// Stop motion immediately
		StopFixedStep();
		PrimaryActorTick.bCanEverTick = false;

		// Disable any further collision
		SetActorEnableCollision(false);
		HitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Play impact if available, else deactivate now
		if (ImpactFlipbook)
		{
			Visual->SetFlipbook(ImpactFlipbook);
			Visual->SetLooping(false);
			Visual->PlayFromStart();
		}
		else
		{
			Deactivate_Internal();
		}
		break;

	default:
		break;
	}
}

void ACPP_ProjectileParent::OnFlipbookFinished()
{
	if (FlipState == EProjectileFlipbookState::Spawn)
	{
		SetFlipbookState(EProjectileFlipbookState::Loop);
		return;
	}

	if (FlipState == EProjectileFlipbookState::Impact)
	{
		Deactivate_Internal(); // return to pool
	}
}

void ACPP_ProjectileParent::TriggerImpactAndDeactivate(const FHitResult& Hit)
{
	if (bHasImpacted) return;
	bHasImpacted = true;

	// Cache impact info (cast away NetQuantize)
	LastImpactPoint = FVector(Hit.ImpactPoint);
	LastImpactNormal = Hit.ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(Hit.ImpactNormal);

	// Enter Impact state: stops motion, disables collision, sets Impact flipbook if any
	SetFlipbookState(EProjectileFlipbookState::Impact, LastImpactPoint, LastImpactNormal);

	// Clear any previous impact timer just in case
	GetWorldTimerManager().ClearTimer(ImpactFinishTimerHandle);

	// If we have an impact flipbook, run a one-shot timer for its duration.
	// Fallback: if no flipbook or duration <= 0, immediately deactivate.
	float Duration = 0.f;

	if (ImpactFlipbook)
	{
		// Total duration of the flipbook in seconds
		Duration = ImpactFlipbook->GetTotalDuration();

		// Respect component play rate if you’re changing it at runtime
		const float PlayRate = (Visual && Visual->GetPlayRate() > 0.f) ? Visual->GetPlayRate() : 1.f;
		if (PlayRate > 0.f)
		{
			Duration = Duration / PlayRate;
		}
	}

	if (Duration > 0.f)
	{
		// Use a normal (time-dilated) timer; set looping=false
		GetWorldTimerManager().SetTimer(
			ImpactFinishTimerHandle,
			this,
			&ACPP_ProjectileParent::OnImpactAnimFinishTimer,
			Duration,
			/*bLoop=*/false
		);
	}
	else
	{
		// No anim (or zero-length): return to pool immediately
		Deactivate_Internal();
	}
}



