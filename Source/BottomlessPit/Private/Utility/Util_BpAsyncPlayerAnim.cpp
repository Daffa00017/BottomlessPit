// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/Util_BpAsyncPlayerAnim.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "AnimSequences/PaperZDAnimSequence.h"  
#include "UObject/SoftObjectPath.h"
#include "Engine/DataTable.h"

UUtil_BpAsyncPlayerAnim* UUtil_BpAsyncPlayerAnim::LoadPlayerAnimRowAsync(UObject* WorldContextObject, UDataTable* InDataTable, FName InRowName, int32 InVariantIndex)
{
    UUtil_BpAsyncPlayerAnim* Action = NewObject<UUtil_BpAsyncPlayerAnim>();
    Action->WorldContextObject = WorldContextObject;
    Action->DataTable = InDataTable;
    Action->RowName = InRowName;
    Action->VariantIndex = InVariantIndex;
    return Action;
}

void UUtil_BpAsyncPlayerAnim::Activate()
{
    if (!DataTable)
    {
        OnFailed.Broadcast();
        return;
    }

    const FPlayerAnimRow* FoundRow = DataTable->FindRow<FPlayerAnimRow>(RowName, TEXT("LoadPlayerAnimRowAsync"));
    if (!FoundRow)
    {
        OnFailed.Broadcast();
        return;
    }

    RowCopy = *FoundRow;

    // Gather all soft refs
    TArray<FSoftObjectPath> Paths;
    auto AddPath = [&Paths](const TSoftObjectPtr<UPaperZDAnimSequence>& Soft)
        {
            if (!Soft.IsNull())
                Paths.Add(Soft.ToSoftObjectPath());
        };

    AddPath(RowCopy.Idle);
    AddPath(RowCopy.Jump_Land);
    AddPath(RowCopy.Jump_Start);
    AddPath(RowCopy.Jump_Falling);
    AddPath(RowCopy.Slide);
    AddPath(RowCopy.Dash);
    AddPath(RowCopy.Wall_Land);
    AddPath(RowCopy.Wall_Idle);
    AddPath(RowCopy.Wall_Mediumfall);
    AddPath(RowCopy.Wall_FastFall);
    AddPath(RowCopy.Walk);
    AddPath(RowCopy.Roll);
    AddPath(RowCopy.Death);
    AddPath(RowCopy.LedgeClimb);
    AddPath(RowCopy.AirSpin);
    AddPath(RowCopy.Run);

    for (auto& Var : RowCopy.Variants)
    {
        AddPath(Var);
    }

    if (Paths.Num() == 0)
    {
        OnAllLoaded();
        return;
    }

    Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Paths,
        FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncPlayerAnim::OnAllLoaded));

    if (!Handle.IsValid())
    {
        OnFailed.Broadcast();
    }
}

void UUtil_BpAsyncPlayerAnim::OnAllLoaded()
{
    FPlayerAnimResolved Resolved;

    Resolved.Idle = RowCopy.Idle.Get();
    Resolved.Jump_Land = RowCopy.Jump_Land.Get();
    Resolved.Jump_Start = RowCopy.Jump_Start.Get();
    Resolved.Jump_Falling = RowCopy.Jump_Falling.Get();
    Resolved.Slide = RowCopy.Slide.Get();
    Resolved.Dash = RowCopy.Dash.Get();
    Resolved.Wall_Land = RowCopy.Wall_Land.Get();
    Resolved.Wall_Idle = RowCopy.Wall_Idle.Get();
    Resolved.Wall_Mediumfall = RowCopy.Wall_Mediumfall.Get();
    Resolved.Wall_FastFall = RowCopy.Wall_FastFall.Get();
    Resolved.Walk = RowCopy.Walk.Get();
    Resolved.Roll = RowCopy.Roll.Get();
    Resolved.Death = RowCopy.Death.Get();
    Resolved.LedgeClimb = RowCopy.LedgeClimb.Get();
    Resolved.AirSpin = RowCopy.AirSpin.Get();
    Resolved.Run = RowCopy.Run.Get();

    for (auto& Var : RowCopy.Variants)
    {
        Resolved.Variants.Add(Var.Get());
    }

    Handle.Reset();

    OnCompleted.Broadcast(RowName, Resolved);
}


