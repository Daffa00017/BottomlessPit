// Util_BpAsyncProjectileFlipbooks.cpp

#include "Utility/Util_BpAsyncProjectileFlipbooks.h"
#include "PaperFlipbook.h"
#include "Engine/DataTable.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UObject/SoftObjectPath.h"

UUtil_BpAsyncProjectileFlipbooks* UUtil_BpAsyncProjectileFlipbooks::LoadProjectileFlipbooksRowAsync(
	UObject* InWorldContextObject, UDataTable* InDataTable, FName InRowName)
{
	UUtil_BpAsyncProjectileFlipbooks* Action = NewObject<UUtil_BpAsyncProjectileFlipbooks>();
	Action->WorldContextObject = InWorldContextObject;
	Action->DataTable = InDataTable;
	Action->TargetRowName = InRowName;
	return Action;
}

void UUtil_BpAsyncProjectileFlipbooks::Activate()
{
	// Validate DataTable and row
	if (!DataTable)
	{
		OnFailed.Broadcast(TargetRowName);
		return;
	}

	const FProjectileAnimRow* Row = DataTable->FindRow<FProjectileAnimRow>(TargetRowName, TEXT("LoadProjectileFlipbooksRowAsync"));
	if (!Row)
	{
		OnFailed.Broadcast(TargetRowName);
		return;
	}

	RowCopy = *Row;

	// Gather any non-null soft refs to stream
	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(3);

	auto AddPath = [&Paths](const TSoftObjectPtr<UPaperFlipbook>& Soft)
		{
			if (!Soft.IsNull())
			{
				Paths.Add(Soft.ToSoftObjectPath());
			}
		};

	AddPath(RowCopy.SpawnFlipbook);
	AddPath(RowCopy.LoopFlipbook);
	AddPath(RowCopy.ImpactFlipbook);

	// If nothing to stream, still succeed (flipbooks can be null)
	if (Paths.Num() == 0)
	{
		OnAllLoaded();
		return;
	}

	Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncProjectileFlipbooks::OnAllLoaded)
	);

	if (!Handle.IsValid())
	{
		OnFailed.Broadcast(TargetRowName);
	}
}

void UUtil_BpAsyncProjectileFlipbooks::Cancel()
{
	if (Handle.IsValid())
	{
		Handle->CancelHandle();
		Handle.Reset();
	}
}

void UUtil_BpAsyncProjectileFlipbooks::OnAllLoaded()
{
	FProjectileAnimResolved Resolved;

	// Resolve hard refs
	Resolved.SpawnFlipbook = RowCopy.SpawnFlipbook.Get();
	Resolved.LoopFlipbook = RowCopy.LoopFlipbook.Get();
	Resolved.ImpactFlipbook = RowCopy.ImpactFlipbook.Get();

	// Copy the nested stats straight through
	Resolved.Stats = RowCopy.Stats;

	Handle.Reset();
	OnCompleted.Broadcast(TargetRowName, Resolved);
}



