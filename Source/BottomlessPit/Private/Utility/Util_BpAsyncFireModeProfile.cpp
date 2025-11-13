// Util_BpAsyncFireModeProfile.cpp


#include "Utility/Util_BpAsyncFireModeProfile.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/DataTable.h"
#include "PaperSprite.h"

UUtil_BpAsyncFireModeProfile* UUtil_BpAsyncFireModeProfile::LoadFireModeRowAsync(UObject* WorldContextObject, UDataTable* FireModeTable, FName RowName)
{
	UUtil_BpAsyncFireModeProfile* Node = NewObject<UUtil_BpAsyncFireModeProfile>();
	Node->WorldContext = WorldContextObject;
	Node->Table = FireModeTable;
	Node->TargetRow = RowName;
	return Node;
}

void UUtil_BpAsyncFireModeProfile::Activate()
{
	if (!IsValid(Table) || !TargetRow.IsValid())
	{
		Fail();
		return;
	}

	if (const FFireModeRow* Found = Table->FindRow<FFireModeRow>(TargetRow, TEXT("LoadFireModeRowAsync")))
	{
		RowCopy = *Found;
		StartStream();
	}
	else
	{
		Fail();
	}
}

void UUtil_BpAsyncFireModeProfile::StartStream()
{
	TArray<FSoftObjectPath> Paths;

	// Pickup sprite
	if (!RowCopy.PickupSprite.IsValid())
	{
		const FSoftObjectPath P = RowCopy.PickupSprite.ToSoftObjectPath();
		if (P.IsValid()) Paths.Add(P);
	}

	// Optional projectile class
	if (!RowCopy.ProjectileClass.IsValid())
	{
		const FSoftObjectPath P = RowCopy.ProjectileClass.ToSoftObjectPath();
		if (P.IsValid()) Paths.Add(P);
	}

	if (Paths.Num() == 0)
	{
		FinishSuccess();
		return;
	}

	FStreamableManager& SM = UAssetManager::GetStreamableManager();
	Handle = SM.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UUtil_BpAsyncFireModeProfile::FinishSuccess),
		FStreamableManager::AsyncLoadHighPriority
	);

	if (!Handle.IsValid())
	{
		Fail();
	}
}

void UUtil_BpAsyncFireModeProfile::FinishSuccess()
{
	FFireModeResolved Resolved;
	Resolved.PickupSprite = RowCopy.PickupSprite.Get();
	Resolved.ProjectileClass = RowCopy.ProjectileClass.Get();
	Resolved.Profile = RowCopy.Profile;

	OnCompleted.Broadcast(TargetRow, Resolved);

	Handle.Reset();
	SetReadyToDestroy();
}

void UUtil_BpAsyncFireModeProfile::Fail()
{
	OnFailed.Broadcast(TargetRow);
	Handle.Reset();
	SetReadyToDestroy();
}

void UUtil_BpAsyncFireModeProfile::Cancel()
{
	if (Handle.IsValid())
	{
		Handle->CancelHandle();
		Handle.Reset();
	}
	SetReadyToDestroy();
}
