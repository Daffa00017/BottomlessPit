// Util_BpAsyncFireModeProfile.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/DataTable.h"
#include "Util_BpAsyncFireModeProfile.generated.h"

class UDataTable;
class UPaperSprite;
struct FStreamableHandle;

/* -----------------------  INLINE TYPES (no separate header)  ----------------------- */

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto    UMETA(DisplayName = "Auto"),
	Semi    UMETA(DisplayName = "Semi"),
	Burst   UMETA(DisplayName = "Burst"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Sniper  UMETA(DisplayName = "Sniper")
};

USTRUCT(BlueprintType)
struct FFireModeProfile
{
	GENERATED_BODY()

	// Identity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	EFireMode Mode = EFireMode::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	FName ProjectileName;

	// Cadence & Ammo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	float RoundsPerMinute = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	int32 MaxAmmo = 0; // 0 = infinite

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	int32 AmmoUsePerTrigger = 1;

	// Counts & Spread
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	int32 ProjectileAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	bool bUseSpread = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (EditCondition = "bUseSpread", ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "30.0"))
	float SpreadAngleDeg = 0.f;


	// Range & Projectile Overrides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (ClampMin = "0.0"))
	float RangeUU = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 2400.f;

	// 0 = derive from Range/Speed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (ClampMin = "0.0"))
	float ProjectileLifeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (ClampMin = "0.01"))
	float ProjectileScale = 1.0f;

	// If true, projectile should NOT auto-impact/deactivate on hit (i.e., penetrate)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	bool bPenetrate = false;

	// Burst-only: spacing between inner shots (0 = same-frame volley)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode", meta = (ClampMin = "0.0"))
	float BurstInterval = 0.05f;

	// Damage per projectile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode")
	float DamagePerProjectile = 1.0f;
};

USTRUCT(BlueprintType)
struct FFireModeRow : public FTableRowBase
{
	GENERATED_BODY()

	// Box visual (soft)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode|Assets")
	TSoftObjectPtr<UPaperSprite> PickupSprite;

	// Optional projectile class override (soft)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode|Assets")
	TSoftClassPtr<AActor> ProjectileClass;

	// Runtime profile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FireMode|Profile")
	FFireModeProfile Profile;
};

USTRUCT(BlueprintType)
struct FFireModeResolved
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireMode|Resolved")
	UPaperSprite* PickupSprite = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireMode|Resolved")
	UClass* ProjectileClass = nullptr; // nullable

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireMode|Resolved")
	FFireModeProfile Profile;
};

/* -----------------------  ASYNC NODE  ----------------------- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFireModeAsyncCompleted, FName, RowName, const FFireModeResolved&, Resolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFireModeAsyncFailed, FName, RowName);

UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncFireModeProfile : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load Fire Mode Row (Async)"), Category = "FireMode|Async")
	static UUtil_BpAsyncFireModeProfile* LoadFireModeRowAsync(UObject* WorldContextObject, UDataTable* FireModeTable, FName RowName);

	UFUNCTION(BlueprintCallable, Category = "FireMode|Async")
	void Cancel();

	UPROPERTY(BlueprintAssignable, Category = "FireMode|Async")
	FFireModeAsyncCompleted OnCompleted;

	UPROPERTY(BlueprintAssignable, Category = "FireMode|Async")
	FFireModeAsyncFailed OnFailed;

	virtual void Activate() override;

private:
	UPROPERTY() UObject* WorldContext = nullptr;
	UPROPERTY() UDataTable* Table = nullptr;
	UPROPERTY() FName      TargetRow;

	FFireModeRow RowCopy;
	TSharedPtr<FStreamableHandle> Handle;

	void StartStream();
	void FinishSuccess();
	void Fail();
};
