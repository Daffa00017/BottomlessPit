// CPP_ProjectileParent.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/Util_BpAsyncProjectileFlipbooks.h" // FProjectileStats / FProjectileAnimResolved
#include "CPP_ProjectileParent.generated.h"

class USphereComponent;
class UPaperFlipbookComponent;
class UPaperFlipbook;

UENUM(BlueprintType)
enum class EProjectileFlipbookState : uint8
{
	None,
	Spawn,
	Loop,
	Impact
};

// Pooling: fired AFTER full deactivation (safe to push back to pool)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileDeactivated, ACPP_ProjectileParent*, Projectile);

// Contact delegates (you handle damage/logic in BP)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileOverlap, ACPP_ProjectileParent*, Projectile, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileBlock, ACPP_ProjectileParent*, Projectile, const FHitResult&, Hit);

/**
 * Pool-friendly, data-driven downward projectile.
 * - Manual swept movement (fixed-step by default; no ProjectileMovement).
 * - Flipbook-only visuals: Spawn -> Loop -> Impact.
 * - On contact: broadcasts delegates (no damage/decisions internally).
 */
UCLASS(Blueprintable)
class BOTTOMLESSPIT_API ACPP_ProjectileParent : public AActor
{
	GENERATED_BODY()

public:
	ACPP_ProjectileParent();

	// ---------- Events ----------
	UPROPERTY(BlueprintAssignable, Category = "Projectile|Events")
	FOnProjectileOverlap OnOverlap;   // Overlap contact (from BeginOverlap)

	UPROPERTY(BlueprintAssignable, Category = "Projectile|Events")
	FOnProjectileBlock OnBlock;       // Blocking contact (from swept move)

	UPROPERTY(BlueprintAssignable, Category = "Projectile|Pooling")
	FOnProjectileDeactivated OnDeactivated;

	// ---------- Config / Setup ----------
	/** Apply resolved flipbooks + stats (from your async loader). Call before FireFrom. */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Config")
	void ApplyRuntimeConfig(const FProjectileAnimResolved& InConfig);

	/** Spawn & arm from any world location with an instigator. */
	void FireFrom(const FVector& StartLocation, AActor* InstigatorActor);

	/** Blueprint convenience: uses Owner or Instigator as instigator. */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Control")
	void FireFromBP(const FVector& StartLocation);

	// ---------- Control / Pooling ----------
	/** Immediately stop and return to pool (skips impact). */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Pooling")
	void DeactivateAndReturnToPool();

	/** Play impact flipbook (if any) then return to pool once it finishes. */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Control")
	void TriggerImpactAndDeactivate(const FHitResult& Hit);

	/** Is this projectile currently active (armed & participating in world)? */
	UFUNCTION(BlueprintPure, Category = "Projectile|State")
	bool IsActive() const { return bActive; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* HitSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperFlipbookComponent* Visual = nullptr;

	// Fallback stats (can be overridden via ApplyRuntimeConfig or by child BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	FProjectileStats Stats;

	// Optional DamageType (not used internally now, but exposed for your BP logic)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Damage")
	TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass();

	// Flipbooks (can be set by ApplyRuntimeConfig or per child BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Flipbooks")
	UPaperFlipbook* SpawnFlipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Flipbooks")
	UPaperFlipbook* LoopFlipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Flipbooks")
	UPaperFlipbook* ImpactFlipbook = nullptr;

	/** If true, the projectile auto-enters Impact state on contact (but still no damage). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Control")
	bool bAutoImpactOnHit = false;

private:
	// Movement core
	void StepMove(float Dt);
	void StartFixedStep();
	void StopFixedStep();

	// Lifecycle
	void ArmAndGo();
	void Disarm();
	void Deactivate_Internal(); // central return-to-pool path (no Destroy)

	// Hit handling
	void HandleBlockingHit(const FHitResult& Hit);

	FTimerHandle ImpactFinishTimerHandle;

	UFUNCTION()
	void OnImpactAnimFinishTimer();

	// Overlap callback
	UFUNCTION()
	void OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// Flipbook FSM
	void SetFlipbookState(EProjectileFlipbookState NewState,
		const FVector& ImpactPoint = FVector::ZeroVector,
		const FVector& ImpactNormal = FVector::UpVector);

	UFUNCTION()
	void OnFlipbookFinished();

	// Runtime state
	bool bActive = false;
	bool bHasImpacted = false;

	// Downward velocity cache
	FVector DownVelocity = FVector::ZeroVector;

	// Distance tracking
	float TraveledDistance = 0.f;

	// Timers
	FTimerHandle LifeTimerHandle;
	FTimerHandle FixedStepTimerHandle;

	// Visual state
	EProjectileFlipbookState FlipState = EProjectileFlipbookState::None;

	// Impact cache (used if you call TriggerImpactAndDeactivate)
	FVector LastImpactPoint = FVector::ZeroVector;
	FVector LastImpactNormal = FVector::UpVector;
};



