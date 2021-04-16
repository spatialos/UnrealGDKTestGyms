// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#include "SnapshotTakingController.h"

#include "Improbable/SpatialGDKSettingsBridge.h"

void ASnapshotTakingController::TakeSnapshot()
{
	ISpatialGDKEditorModule* SpatialGDKEditorModule = FModuleManager::GetModulePtr<ISpatialGDKEditorModule>("SpatialGDKEditor");
	if (SpatialGDKEditorModule != nullptr)
	{
		UWorld* World = GetWorld();
		SpatialGDKEditorModule->TakeSnapshot(World, [this](bool bSuccess, const FString& PathToSnapshot) {
			OnSnapshotOutcome(bSuccess, PathToSnapshot);
    });
	}
}

void ASnapshotTakingController::OnSnapshotOutcome_Implementation(bool bSuccess, const FString& Path)
{
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Snapshot taken at path %s"), *Path);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Snapshot taking failed"));
	}
}
