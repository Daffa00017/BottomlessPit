#include "Utility/Util_BpAsyncFont.h"
#include "Engine/StreamableManager.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"

UUtil_BpAsyncFont* UUtil_BpAsyncFont::LoadFontRowAsync(UObject* WorldContextObject, UDataTable* InDataTable, FName InRowName)
{
    UUtil_BpAsyncFont* Action = NewObject<UUtil_BpAsyncFont>();
    Action->WorldContextObject = WorldContextObject;
    Action->DataTable = InDataTable;
    Action->RowName = InRowName;
    return Action;
}

void UUtil_BpAsyncFont::Activate()
{
    if (!DataTable)
    {
        OnFailed.Broadcast();
        return;
    }

    const Fs_FontRow* Row = DataTable->FindRow<Fs_FontRow>(RowName, TEXT("UUtil_BpAsyncFont::Activate"));
    if (!Row)
    {
        OnFailed.Broadcast();
        return;
    }

    RowCopy = *Row;

    // Collect soft refs
    TArray<FSoftObjectPath> AssetsToStream;
    if (!RowCopy.Font.IsNull())
    {
        AssetsToStream.Add(RowCopy.Font.ToSoftObjectPath());
    }
    if (!RowCopy.FontFace.IsNull())
    {
        AssetsToStream.Add(RowCopy.FontFace.ToSoftObjectPath());
    }

    if (AssetsToStream.Num() == 0)
    {
        OnAllLoaded();
        return;
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    Handle = Streamable.RequestAsyncLoad(
        AssetsToStream,
        FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncFont::OnAllLoaded)
    );
}

void UUtil_BpAsyncFont::OnAllLoaded()
{
    Fs_FontResolved Resolved;
    Resolved.Font = RowCopy.Font.Get();
    Resolved.FontFace = RowCopy.FontFace.Get();

    if (Resolved.Font || Resolved.FontFace)
    {
        OnCompleted.Broadcast(RowName, Resolved);
    }
    else
    {
        OnFailed.Broadcast();
    }

    Handle.Reset();
}



