// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/Util_BpAsyncSFX.h"
#include "Sound/SoundCue.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/DataTable.h"

UUtil_BpAsyncSFX* UUtil_BpAsyncSFX::LoadSFXDataRowAsync(
    UObject* InWorldContextObject, UDataTable* InDataTable, FName InRowName)
{
    UUtil_BpAsyncSFX* Action = NewObject<UUtil_BpAsyncSFX>();
    Action->WorldContextObject = InWorldContextObject;
    Action->DataTable = InDataTable;
    Action->RowName = InRowName;
    return Action;
}

void UUtil_BpAsyncSFX::Activate()
{
    if (!DataTable)
    {
        OnFailed.Broadcast();
        return;
    }

    // Find the row in the DataTable
    const FSFXDataRow* Row = DataTable->FindRow<FSFXDataRow>(RowName, TEXT("LoadSFXDataRowAsync"));
    if (!Row)
    {
        OnFailed.Broadcast();
        return;
    }

    // Make a copy of the row data
    RowCopy = *Row;

    // Gather all soft paths to load. In this case, just the one sound.
    TArray<FSoftObjectPath> Paths;
    if (!RowCopy.Sound.IsNull())
    {
        Paths.Add(RowCopy.Sound.ToSoftObjectPath());
    }

    if (Paths.Num() == 0)
    {
        // Nothing to load, but still broadcast success with the data (Sound will be null)
        OnAllLoaded();
        return;
    }

    // Request the async load
    Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Paths,
        FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncSFX::OnAllLoaded));

    if (!Handle.IsValid())
    {
        OnFailed.Broadcast();
    }
}

void UUtil_BpAsyncSFX::OnAllLoaded()
{
    // Create the resolved struct to pass to the output delegate
    FSFXDataResolved Resolved;
    Resolved.Sound = RowCopy.Sound.Get(); // Get the hard reference
    Resolved.DescriptionName = RowCopy.DescriptionName; // Copy the name

    Handle.Reset();
    OnCompleted.Broadcast(RowName, Resolved);
}
