// Util_BpAsyncProjectileFlipbooks.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/DataTable.h"
#include "Util_BpAsyncProjectileFlipbooks.generated.h"

class UDataTable;
class UPaperFlipbook;
struct FStreamableHandle;

/** Minimal, arcade-centric stat block for projectiles (clean & readable) */
USTRUCT(BlueprintType)
struct FProjectileStats
{
	GENERATED_BODY()

	// Movement / lifespan
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Speed = 2400.f; // uu/s downward

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxLifeSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxTravelDistance = 1800.f;

	// Damage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 1.f;

	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FName EnemyChannelName = FName(TEXT("Enemy"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float CollisionRadius = 10.f;

	// Stepping (tick-less preferred)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bUseFixedStep = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float FixedStepHz = 120.f;
};

/**
 * DataTable Row: SOFT refs for 3 flipbook phases + nested Stats.
 * This is the ONLY struct you need when creating the Data Table asset in-editor.
 */
USTRUCT(BlueprintType)
struct FProjectileAnimRow : public FTableRowBase
{
	GENERATED_BODY()

	// Flipbook soft refs (spawn one-shot, in-flight loop, impact one-shot)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flipbooks")
	TSoftObjectPtr<UPaperFlipbook> SpawnFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flipbooks")
	TSoftObjectPtr<UPaperFlipbook> LoopFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flipbooks")
	TSoftObjectPtr<UPaperFlipbook> ImpactFlipbook;

	// Tuning bundle for readability
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FProjectileStats Stats;
};

/** Resolved (hard) refs after streaming + a copy of the Stats */
USTRUCT(BlueprintType)
struct FProjectileAnimResolved
{
	GENERATED_BODY()

	// Hard refs (can be nullptr if absent in row)
	UPROPERTY(BlueprintReadOnly, Category = "Flipbooks")
	UPaperFlipbook* SpawnFlipbook = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Flipbooks")
	UPaperFlipbook* LoopFlipbook = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Flipbooks")
	UPaperFlipbook* ImpactFlipbook = nullptr;

	// Same stats bundled for direct apply on the projectile
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FProjectileStats Stats;
};

// BP Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FProjFlipbooksLoadOK, FName, RowName, const FProjectileAnimResolved&, Config);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProjFlipbooksFailed, FName, RowName);

/**
 * Single async node to stream the 3 flipbooks from a Data Table row (FProjectileAnimRow).
 * Appears in BP as: "Load Projectile Flipbooks Row (Async)".
 */
UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncProjectileFlipbooks : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Success(RowName, ResolvedConfig) */
	UPROPERTY(BlueprintAssignable, Category = "Async")
	FProjFlipbooksLoadOK OnCompleted;

	/** Failed(RowName) */
	UPROPERTY(BlueprintAssignable, Category = "Async")
	FProjFlipbooksFailed OnFailed;

	/** Kick off the async load for a given Data Table row */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject",
		DisplayName = "Load Projectile Flipbooks Row (Async)"), Category = "Async")
	static UUtil_BpAsyncProjectileFlipbooks* LoadProjectileFlipbooksRowAsync(
		UObject* WorldContextObject, UDataTable* DataTable, FName RowName);

	// UBlueprintAsyncActionBase
	virtual void Activate() override;

	/** Optional: cancel streaming if you no longer need results */
	UFUNCTION(BlueprintCallable, Category = "Async")
	void Cancel();

private:
	void OnAllLoaded();

private:
	// Keep refs so GC doesn't kill us mid-stream
	UPROPERTY() UObject* WorldContextObject = nullptr;
	UPROPERTY() UDataTable* DataTable = nullptr;
	UPROPERTY() FName TargetRowName;

	// Copy of the row we found (soft refs + stats)
	FProjectileAnimRow RowCopy;

	// Stream handle lives on this UObject
	TSharedPtr<FStreamableHandle> Handle;
};





