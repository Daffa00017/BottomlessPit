// GunParent Header

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "GameFramework/DamageType.h"
#include "CPP_Gun.generated.h"

UCLASS()
class BOTTOMLESSPIT_API ACPP_Gun : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPP_Gun();


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gun")
	TObjectPtr<UPaperFlipbookComponent> GunFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gun|Config")
	TObjectPtr<UPaperFlipbook> DefaultGunFlipbook;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gun")
	TObjectPtr<USceneComponent> GunPivot;

	// New: muzzle scene for start location
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gun")
	TObjectPtr<USceneComponent> Muzzle;

	// Hitscan config
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	float TraceRange = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	TEnumAsByte<ECollisionChannel> FireTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire|Debug")
	bool bDrawDebug = false;

	// Core hitscan callable from BP (your BP interface will call this)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CoreFireFromMuzzle(const FVector& AimDir);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FVector GetMuzzleWorldLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CoreHitscanFromMuzzle_PastCursor();

	// add this toggle & small epsilon if you like
	UPROPERTY(EditAnywhere, Category = "Fire")
	bool bPreventBackwardShots = true;

	UPROPERTY(EditAnywhere, Category = "Fire", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float MirrorDeadZoneDeg = 3.f; // small grace near boundary

	UPROPERTY(EditAnywhere, Category = "Fire|AimGate")
	float NearCursorThreshold = 12.f;   // cursor too close to muzzle? snap forward

	UPROPERTY(EditAnywhere, Category = "Fire|AimGate", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float MinFrontDot = 0.10f; // how "in front" the cursor must be (0=any, 0.1>84° cone)

	UPROPERTY(EditAnywhere, Category = "Fire|AimGate")
	float FrontGateDist = 24.f;   // uu required along-forward distance to aim at cursor

	static void MirrorIntoMuzzleFrontHemisphere(const AActor* Owner, const FVector& MuzzleWorld,FVector& DeltaWorld, float DeadZoneDeg);

	UFUNCTION(BlueprintNativeEvent, Category = "Fire")
	void ShootOnHit(FHitResult ShootResult);

	//Hit Event Variable

	UPROPERTY(EditAnywhere, Category = "Fire|HitVariable")
	float Damage = 24.f;

	UPROPERTY(EditAnywhere, Category = "Fire|HitVariable")
	TSubclassOf<UDamageType> DamageTypeClass;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool GetCursorWorldOnOwnerPlane(FVector& OutWorld) const;
	

};



