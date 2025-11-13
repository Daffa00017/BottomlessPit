// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/DataTable.h"
#include "Util_BpAsyncSFX.generated.h"

class UDataTable;
class USoundCue;
struct FStreamableHandle;

/** * DataTable Row: Contains a soft reference to a Sound Cue and a Name.
 */
USTRUCT(BlueprintType)
struct FSFXDataRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USoundCue> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName DescriptionName;
};

/** * Resolved/hard references after async load. This is what's output on success.
 */
USTRUCT(BlueprintType)
struct FSFXDataResolved
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    USoundCue* Sound = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FName DescriptionName;
};

// Delegates for BP
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFXLoadOK, FName, RowName, const FSFXDataResolved&, SFX);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFXFailed);


UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncSFX : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
    /** Completed(RowName, ResolvedSFX) */
    UPROPERTY(BlueprintAssignable, Category = "Async")
    FSFXLoadOK OnCompleted;

    /** Failed() */
    UPROPERTY(BlueprintAssignable, Category = "Async")
    FSFXFailed OnFailed;

    /** Starts the async load */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load SFX Data Row (Async)"))
    static UUtil_BpAsyncSFX* LoadSFXDataRowAsync(UObject* WorldContextObject, UDataTable* DataTable, FName RowName);

    virtual void Activate() override;

private:
    void OnAllLoaded();

private:
    UPROPERTY() UObject* WorldContextObject = nullptr;
    UPROPERTY() UDataTable* DataTable = nullptr;
    UPROPERTY() FName RowName;

    FSFXDataRow RowCopy;
    TSharedPtr<FStreamableHandle> Handle;
};
