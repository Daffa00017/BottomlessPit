//BlpEnemies Header

#pragma once

#include "CoreMinimal.h"
#include "Pawn/Enemy/CPP_EnemyParent.h"
#include "CPP_BlpEnemies.generated.h"

UENUM(BlueprintType)
enum class EBPEnemyKind : uint8 { Walker, Flyer };

// Axis to patrol on (matches your side-scroller lanes)
UENUM(BlueprintType)
enum class EBlpLaneAxis : uint8 { X, Y };

UCLASS()
class BOTTOMLESSPIT_API ACPP_BlpEnemies : public ACPP_EnemyParent
{
	GENERATED_BODY()

public:
	ACPP_BlpEnemies();
	// Pool hooks (must match parent signatures exactly)
	virtual void ActivateFromPool(const FVector& WorldPos) override;
	virtual void DeactivateToPool() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Optional no-op stubs to avoid breaking any existing BP/Generator calls.
	// Keep for now; we can delete or implement later.
	UFUNCTION(BlueprintCallable) void StartSimpleAI() {}
	UFUNCTION(BlueprintCallable) void StopSimpleAI() {}

	// --- Debug (walker/flyer safe) ---
	UPROPERTY(EditAnywhere, Category = "MinAI|Debug")
	bool bDebugAI = true;

	UPROPERTY(EditAnywhere, Category = "MinAI|Debug", meta = (ClampMin = "0.0"))
	float DebugLineTime = 0.f;

protected:

	// --- Tunables (mirrors the BT task style) ---
	UPROPERTY(EditAnywhere, Category = "MinAI") EBPEnemyKind Kind = EBPEnemyKind::Walker;
	UPROPERTY(EditAnywhere, Category = "MinAI") EBlpLaneAxis LaneAxis = EBlpLaneAxis::X; // BT used X

	UPROPERTY(EditAnywhere, Category = "MinAI") float MoveSpeed = 300.f;

	// “probes” like in your BT: short forward look + down ground check
	UPROPERTY(EditAnywhere, Category = "MinAI") float WallLookAheadUU = 28.f;   // forward wall check
	UPROPERTY(EditAnywhere, Category = "MinAI") float GroundAheadUU = 36.f;   // walker: forward point to check ground
	UPROPERTY(EditAnywhere, Category = "MinAI") float GroundDownUU = 60.f;   // walker: drop distance
	UPROPERTY(EditAnywhere, Category = "MinAI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkableMinNormalZ = 0.5f; // must be this “flat” to count as ground

	// flyer hover (optional)
	UPROPERTY(EditAnywhere, Category = "MinAI|Flyer") float HoverAmp = 12.f;
	UPROPERTY(EditAnywhere, Category = "MinAI|Flyer") float HoverHz = 0.8f;

	// debounce like your BT Cooldown node
	UPROPERTY(EditAnywhere, Category = "MinAI") float FlipCooldownSeconds = 0.12f;

	// Use the **same channel as your BT task** (default Visibility)
	UPROPERTY(EditAnywhere, Category = "MinAI|Collision")
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_Visibility;

private:
	// state
	bool  bAIOn = false;
	int32 Dir = 1;             // +1 right, -1 left along the chosen lane axis
	float FlipCooldown = 0.f;
	float BaseZ = 0.f;
	FVector AxisVec = FVector::ForwardVector; // world axis unit (X or Y)

	// helpers
	void ConfigureCollisionForOverlapPlayer();
	bool WallAhead() const;
	bool WalkableGroundAt(const FVector& XY, float DownDist, FVector& OutHitPoint) const;

	void WalkerTick(float dt);
	void FlyerTick(float dt);

	
};



