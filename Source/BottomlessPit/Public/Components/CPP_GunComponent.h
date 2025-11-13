// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/Util_BpAsyncFireModeProfile.h"
#include "CPP_GunComponent.generated.h"


UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class BOTTOMLESSPIT_API UCPP_GunComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCPP_GunComponent();

	// ===== PUBLIC BP API (only these) =====
	UFUNCTION(BlueprintCallable, Category = "Gun") void StartFire();
	UFUNCTION(BlueprintCallable, Category = "Gun") void StopFire();
	UFUNCTION(BlueprintCallable, Category = "Gun") void FireOnce();     // semi tap (RPM gated)

	/** Set active mode + projectile row (call this from your pickup overlap). */
	UFUNCTION(BlueprintCallable, Category = "Gun|FireMode")
	void ApplyFireMode(const FFireModeProfile& Profile, FName ProjectileRowName);

	/** Optional helper if you want to change RPM at runtime directly. */
	UFUNCTION(BlueprintCallable, Category = "Gun|FireMode")
	void SetRoundsPerMinute(float NewRPM);

	// ===== ONE event you implement in BP to call your pooled-spawn interface =====
	/** Called by C++ for every single shot that must be spawned.
		Implement in BP: forward to your GameMode pool interface with RowName + ShotDir. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Gun|Fire")
	void OnRequestSpawnShot(FVector ShotDir, FName ProjectileRowName, bool bPenetrate, float ProjectileScale, float DamagePerProjectile, float ProjectileLifeSecondsOverride);

	UFUNCTION(BlueprintCallable, Category = "Gun | Profile")
	FFireModeProfile GetCurrentProfile() const { return  CurrentProfile; }

	UFUNCTION(BlueprintNativeEvent, Category = "Gun|Ammo")
	bool TryConsumeAmmo(int32 Cost);
	virtual bool TryConsumeAmmo_Implementation(int32 Cost);

private:
	// cadence
	UPROPERTY(EditAnywhere, Category = "Gun|FireMode")
	float RoundsPerMinute = 600.f;

	FTimerHandle FireTimer;
	FTimerHandle BurstTimer;
	double LastShotTime = -1.0;

	FORCEINLINE float ShotInterval() const { return (RoundsPerMinute > 0.f) ? (60.f / RoundsPerMinute) : 0.f; }

	// active mode state (pure data)
	UPROPERTY(EditAnywhere, Category = "Gun|FireMode", meta = (AllowPrivateAccess = "true"))
	FFireModeProfile CurrentProfile;

	UPROPERTY(EditAnywhere, Category = "Gun|FireMode", meta = (AllowPrivateAccess = "true"))
	FName CurrentProjectileRowName;

	// internal logic
	void FireAccordingToMode();           // C++ handles mode branching
	void FireVolley(int32 Count);         // spawns N “shots” via OnRequestSpawnShot
	void GenerateShotDirs(int32 Count, TArray<FVector>& OutDirs) const;
	void FireTrigger();

	// burst state
	bool  bBurstActive = false;
	int32 BurstShotsRemaining = 0;
	void  BeginBurst();
	void  BurstTick();
		
};



