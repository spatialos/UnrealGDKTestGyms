// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "WorkerSDK/improbable/c_worker.h"

template <typename TStream>
struct SnapshotHelper
{
public:
	bool IsStreamValid()
	{
		Worker_SnapshotState SnapshotState = GetStreamState(Stream);
		const bool bStreamIsValid = SnapshotState.stream_state == Worker_StreamState::WORKER_STREAM_STATE_GOOD;
		if (!bStreamIsValid)
		{
			UE_LOG(LogSnapshotMigration, Error, TEXT("Snapshot stream error: %s"), UTF8_TO_TCHAR(SnapshotState.error_message));
		}
		return bStreamIsValid;
	}

	virtual ~SnapshotHelper()
	{
		CloseStream(Stream);
	}

protected:
	SnapshotHelper(const FString& Path, TStream* (*InOpenStream)(const char*, const Worker_SnapshotParameters*), Worker_SnapshotState (*InGetStreamState)(TStream*), void (*InCloseStream)(TStream*))
	{
		const Worker_ComponentVtable DefaultVTable{};
		Worker_SnapshotParameters Parameters{};
		Parameters.default_component_vtable = &DefaultVTable;

		Stream = InOpenStream(TCHAR_TO_UTF8(*Path), &Parameters);
		GetStreamState = InGetStreamState;
		CloseStream = InCloseStream;
	}

	TStream* Stream;

private:
	Worker_SnapshotState (*GetStreamState)(TStream*);
	void (*CloseStream)(TStream*);
};

using Components = TArray<Worker_ComponentData>;
using Entity = TTuple<Worker_EntityId, Components>;
using Entities = TMap<Worker_EntityId, Components>;

struct InputSnapshotHelper : public SnapshotHelper<Worker_SnapshotInputStream>
{
public:
	InputSnapshotHelper(const FString& Path);

	virtual ~InputSnapshotHelper() override;

	bool ReadEntities(Entities& OutEntities);

private:
	TArray<Worker_ComponentData*> AcquiredComponentData;
};

struct OutputSnapshotHelper : public SnapshotHelper<Worker_SnapshotOutputStream>
{
public:
	OutputSnapshotHelper(const FString& Path);

	bool WriteEntitiesToSnapshot(const Entities& EntitiesToWrite);
};
