#pragma once

#include "CoreMinimal.h"
#include "Engine/FontFace.h"
#include "Engine/DataTable.h"
//#include "Fonts/SlateFontInfo.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Util_BpAsyncFont.generated.h"

class UDataTable;
class UFont;
class UFontFace;
//class FSlateFontInfo;
struct FStreamableHandle;

/** Row type for DataTable (soft references) */
USTRUCT(BlueprintType)
struct Fs_FontRow : public FTableRowBase
{
    GENERATED_BODY()

    // The actual font asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    TSoftObjectPtr<UFont> Font;

    // Optional: font face asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    TSoftObjectPtr<UFontFace> FontFace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    FName FontName;

};

/** Resolved fonts after async load */
USTRUCT(BlueprintType)
struct Fs_FontResolved
{
    GENERATED_BODY()

    // The actual font asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    UFont* Font;

    // Optional: font face asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    UFontFace* FontFace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
    FName FontName;
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFontLoadOK, FName, RowName, const Fs_FontResolved&, Font);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFontFailed);

/**
 * Async font loader
 */
UCLASS()
class BOTTOMLESSPIT_API UUtil_BpAsyncFont : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Async")
    FFontLoadOK OnCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Async")
    FFontFailed OnFailed;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load Font Row (Async)"))
    static UUtil_BpAsyncFont* LoadFontRowAsync(UObject* WorldContextObject, UDataTable* DataTable, FName RowName);

    virtual void Activate() override;

private:
    void OnAllLoaded();

    UPROPERTY() UObject* WorldContextObject = nullptr;
    UPROPERTY() UDataTable* DataTable = nullptr;
    UPROPERTY() FName RowName;

    Fs_FontRow RowCopy;
    TSharedPtr<FStreamableHandle> Handle;
};



