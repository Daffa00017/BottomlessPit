// Util_BpAsyncVFXFlipbooks.cpp

#include "Utility/Util_BpAsyncVFXFlipbooks.h"
#include "PaperFlipbook.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/DataTable.h"

UUtil_BpAsyncVFXFlipbooks* UUtil_BpAsyncVFXFlipbooks::LoadVFXFlipbooksRowAsync(
    UObject* InWorldContextObject, UDataTable* InDataTable, FName InRowName)
{
    UUtil_BpAsyncVFXFlipbooks* Action = NewObject<UUtil_BpAsyncVFXFlipbooks>();
    Action->WorldContextObject = InWorldContextObject;
    Action->DataTable = InDataTable;
    Action->RowName = InRowName;
    return Action;
}

void UUtil_BpAsyncVFXFlipbooks::Activate()
{
    if (!DataTable)
    {
        OnFailed.Broadcast();
        return;
    }

    const FVFXAnimationRow* Row = DataTable->FindRow<FVFXAnimationRow>(RowName, TEXT("LoadVFXFlipbooksRowAsync"));
    if (!Row)
    {
        OnFailed.Broadcast();
        return;
    }

    RowCopy = *Row;

    // Gather all soft refs
    TArray<FSoftObjectPath> Paths;
    auto AddPath = [&Paths](const TSoftObjectPtr<UPaperFlipbook>& Soft)
        {
            if (!Soft.IsNull())
            {
                Paths.Add(Soft.ToSoftObjectPath());
            }
        };

    AddPath(RowCopy.AnimVar1);
    AddPath(RowCopy.AnimVar2);
    AddPath(RowCopy.AnimVar3);

    if (Paths.Num() == 0)
    {
        // Nothing to load, still broadcast with nulls (safe)
        OnAllLoaded();
        return;
    }

    Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Paths,
        FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncVFXFlipbooks::OnAllLoaded));

    if (!Handle.IsValid())
    {
        OnFailed.Broadcast();
    }
}

void UUtil_BpAsyncVFXFlipbooks::OnAllLoaded()
{
    FVFXAnimationResolved Resolved;
    Resolved.AnimVar1 = RowCopy.AnimVar1.Get();
    Resolved.AnimVar2 = RowCopy.AnimVar2.Get();
    Resolved.AnimVar3 = RowCopy.AnimVar3.Get();
    Resolved.Tint = RowCopy.Tint;

    Handle.Reset();
    OnCompleted.Broadcast(RowName, Resolved);
}



