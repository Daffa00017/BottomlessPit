// Util_BpAsyncVFXFlipbooks.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/DataTable.h"
#include "Util_BpAsyncVFXFlipbooks.generated.h"

class UDataTable;
class UPaperFlipbook;
struct FStreamableHandle;

/** DataTable Row: SOFT references to flipbooks (edit this if you add more vars) */
USTRUCT(BlueprintType)
struct FVFXAnimationRow : public FTableRowBase
{
    GENERATED_BODY()

    // Match your BP row intent: 3 soft flipbooks
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UPaperFlipbook> AnimVar1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UPaperFlipbook> AnimVar2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UPaperFlipbook> AnimVar3;
};

/** Resolved/hard refs after async load */
USTRUCT(BlueprintType)
struct FVFXAnimationResolved
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    UPaperFlipbook* AnimVar1 = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UPaperFlipbook* AnimVar2 = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UPaperFlipbook* AnimVar3 = nullptr;
};

// Delegates for BP
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVFXLoadOK, FName, RowName, const FVFXAnimationResolved&, VFX);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVFXFailed);

/**
 * Async BP node: loads a DataTable row of soft PaperFlipbook refs, outputs hard refs.
 * In BP it appears as: "Load VFX Flipbooks Row (Async)"
 */
UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncVFXFlipbooks : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    /** Completed(RowName, ResolvedFlipbooks) */
    UPROPERTY(BlueprintAssignable, Category = "Async")
    FVFXLoadOK OnCompleted;

    /** Failed() */
    UPROPERTY(BlueprintAssignable, Category = "Async")
    FVFXFailed OnFailed;

    /** Starts the async load */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load VFX Flipbooks Row (Async)"))
    static UUtil_BpAsyncVFXFlipbooks* LoadVFXFlipbooksRowAsync(UObject* WorldContextObject, UDataTable* DataTable, FName RowName);

    virtual void Activate() override;

private:
    void OnAllLoaded();

private:
    UPROPERTY() UObject* WorldContextObject = nullptr;
    UPROPERTY() UDataTable* DataTable = nullptr;
    UPROPERTY() FName RowName;

    FVFXAnimationRow RowCopy;
    TSharedPtr<FStreamableHandle> Handle;
};



