// Visual Effects CPP Header.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbook.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CPP_FVX.generated.h"

UCLASS()
class BOTTOMLESSPIT_API ACPP_FVX : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPP_FVX();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UPaperFlipbookComponent> VFXFlipbook;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<USceneComponent> VFXPivot;

	UPROPERTY(EditAnywhere, Category = "VFX|Config")
	bool bVFXLooping = false;

	UFUNCTION(BlueprintNativeEvent, Category = "FVX")
	void PoolObjectToGameMode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<class UPaperSpriteComponent> TrailSprite;

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float TileWorldUU = 16.f;          // world uu per dash tile (Tiling = Len / TileWorldUU)

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float Thickness = 1.0f;            // vertical fatness (your Core thickness)

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float DashFill = 0.90f;            // fraction of each tile that's on (0..1)

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float HeadTaper = 0.2f;            // 0 = no taper, higher = more muzzle/impact taper

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float TrailLife = 0.06f;           // fade duration

	// optional solid/dashed switch for your DashMask
	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	bool bUseDashes = true;

	// If you want strict 2D (X–Z plane), clamp Y to this plane (otherwise set to NAN to skip)
	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float ForcePlaneY = NAN;

	// optional: pixel-based scaling (use either this OR GetLocalBounds)
	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float TrailSourceWidthPx = 16.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float PixelsPerUnit = 1.f;         // project PPU for this sprite

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float StartTrimUU = 0.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float EndTrimUU = 1.5f;    // ~1–2 uu usually enough (depends on your softness)

	// Overlap each dash this much in world uu (0 = normal gaps, >0 = negative gap)
	UPROPERTY(EditAnywhere, Category = "VFX|Trail")
	float DashOverlapUU = 1.0f;   // try 0.5..2.0 uu

	// Tell code what pivot your sprite asset uses
	UPROPERTY(EditAnywhere, Category = "VFX|Trail") 
	bool  bSpritePivotIsCenter = false;


	UFUNCTION(BlueprintCallable, Category = "VFX")
	void ActivateTrail(const FVector& From, const FVector& To);

	void SetTrailVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void ActivateBulletImpact(UPaperFlipbook* BurstAnim, const FVector& Muzzle, const FVector& Impact);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void ActivateVFX(UPaperFlipbook* Anim, const FVector& ImpactLoc);

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void DeActivateVFX();

	FVector OriginLoc = FVector(0.0f, 0.0f, 0.0f);
	

private:	
	FTimerHandle FVXTimer;
	FTimerHandle TrailFadeTimer;

	// Helper: face from Player to ImpactLoc (matches your FindLookAtRotation path)
	FRotator ComputeLookAtFromPlayer(const FVector& Target) const;

	// Small pad added to Flipbook length before deactivation
	UPROPERTY(EditAnywhere, Category = "FVX|Config")
	float ExtraHoldSeconds = 0.05f;

	class UMaterialInstanceDynamic* TrailMID = nullptr;

	float CachedBaseLenLocalUU = 1.f;   // unscaled local width of the sprite in UU
	bool  bTrailReady = false;

	static FVector SnapToPixel(const FVector& V, float PPU)
	{
		const float Grid = 1.f / FMath::Max(PPU, 0.001f);
		return FVector(FMath::GridSnap(V.X, Grid), V.Y, FMath::GridSnap(V.Z, Grid));
	}

	static float DashMixFromDir2D(const FVector& Dir, float SolidAngleDeg, float DashedAngleDeg)
	{
		const float ang = FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(Dir.Z, Dir.X)));
		const float t = FMath::GetRangePct(SolidAngleDeg, DashedAngleDeg, ang);
		return FMath::Clamp(t, 0.f, 1.f);
	}
};



