// Util_BpAsyncPlayerAnim.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/DataTable.h"
//#include "PaperZDAnimSequence.h"
#include "Util_BpAsyncPlayerAnim.generated.h"

class UPaperZDAnimSequence;
class UDataTable;
struct FStreamableHandle; // forward is okay; include in .cpp

/** Row struct (soft references, used in DataTable) */
USTRUCT(BlueprintType)
struct FPlayerAnimRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Idle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Jump_Land;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Jump_Start;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Jump_Falling;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Slide;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Dash;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Wall_Land;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Wall_Idle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Wall_Mediumfall;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Wall_FastFall;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Walk;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Roll;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Death;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> LedgeClimb;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> AirSpin;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperZDAnimSequence> Run;

    // Optional variants for skins, extra anims
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UPaperZDAnimSequence>> Variants;
};

/** Resolved struct (hard refs after async load) */
USTRUCT(BlueprintType)
struct FPlayerAnimResolved
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Idle = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Jump_Land = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Jump_Start = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Jump_Falling = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Slide = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Dash = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Wall_Land = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Wall_Idle = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Wall_Mediumfall = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Wall_FastFall = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Walk = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Roll = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Death = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* LedgeClimb = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* AirSpin = nullptr;
    UPROPERTY(BlueprintReadOnly) UPaperZDAnimSequence* Run = nullptr;

    UPROPERTY(BlueprintReadOnly) TArray<UPaperZDAnimSequence*> Variants;
};

// Dynamic multicast delegates for BP
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLoadOK, FName, RowName, const FPlayerAnimResolved&, Anim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFailed);

/**
 * Async node: loads a DataTable row of soft PaperZD sequences, resolves them, and returns hard refs.
 */
UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncPlayerAnim : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Async") FLoadOK OnCompleted;
    UPROPERTY(BlueprintAssignable, Category = "Async") FFailed OnFailed;

    // NOTE: don't use INDEX_NONE default here; set via CPP_Default_ meta
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load Player Anim Row (Async)", CPP_Default_VariantIndex = "-1"))
    static UUtil_BpAsyncPlayerAnim* LoadPlayerAnimRowAsync(UObject* WorldContextObject, UDataTable* DataTable, FName RowName, int32 VariantIndex);

    virtual void Activate() override;

private:
    void OnAllLoaded();

    UPROPERTY() UObject* WorldContextObject = nullptr;
    UPROPERTY() UDataTable* DataTable = nullptr;

    UPROPERTY() FName RowName;
    UPROPERTY() int32 VariantIndex = -1;

    FPlayerAnimRow RowCopy;
    TSharedPtr<FStreamableHandle> Handle;
};



