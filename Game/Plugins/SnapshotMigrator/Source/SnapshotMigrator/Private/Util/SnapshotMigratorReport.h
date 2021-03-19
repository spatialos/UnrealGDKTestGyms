// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Misc/StringBuilder.h"
#include "WorkerSDK/improbable/c_worker.h"

enum class EMigrationResult : uint8
{
	Invalid,
	FailedFiltering,
	PassedFiltering,
	Success,
	Skipped,
	Failed
};

enum class EEntityClassification : uint8
{
	Invalid,
	System,
	Startup,
	Dynamic
};

struct FMigrationRecord
{
	virtual ~FMigrationRecord()
	{
	}

	virtual void ToString(FStringBuilderBase& StringBuilder) const;

	FString ResultReason;
	EMigrationResult Result = EMigrationResult::Invalid;
};

struct FFieldMigrationRecord : public FMigrationRecord
{
	virtual void ToString(FStringBuilderBase& StringBuilder) const override;
	bool operator<(const FFieldMigrationRecord& Other) const
	{
		return TargetFieldId < Other.TargetFieldId;
	}

	FString FieldName;
	uint32 SourceFieldId = 0;
	uint32 TargetFieldId = 0;
};

struct FComponentMigrationRecord : public FMigrationRecord
{
	virtual void ToString(FStringBuilderBase& StringBuilder) const override;
	bool operator<(const FComponentMigrationRecord& Other) const
	{
		return TargetComponentId < Other.TargetComponentId;
	}

	TArray<FFieldMigrationRecord> FieldMigrationRecords;

	Worker_ComponentId SourceComponentId = 0;
	Worker_ComponentId TargetComponentId = 0;
};

struct FEntityMigrationRecord : public FMigrationRecord
{
	virtual void ToString(FStringBuilderBase& StringBuilder) const override;

	TArray<FComponentMigrationRecord> ComponentMigrationRecords;
	Worker_EntityId SourceEntityId = 0;
	EEntityClassification Classification = EEntityClassification::Invalid;
};

class SnapshotMigratorReport
{
public:
	SnapshotMigratorReport(const int32 EntityCount);

	void OutputReportToLog();

	FEntityMigrationRecord& AddEntityMigrationRecord(const Worker_EntityId EntityId);
	FEntityMigrationRecord& GetEntityMigrationRecord(const Worker_EntityId EntityId);

	static inline FString EntityClassificationToString(const EEntityClassification EntityType)
	{
		switch (EntityType)
		{
			case EEntityClassification::Invalid:
				return TEXT("Invalid");
			case EEntityClassification::System:
				return TEXT("System");
			case EEntityClassification::Startup:
				return TEXT("Startup");
			case EEntityClassification::Dynamic:
				return TEXT("Dynamic");
		}

		return FString();
	}

	static inline FString MigrationResultToString(const EMigrationResult MigrationResult)
	{
		switch (MigrationResult)
		{
			case EMigrationResult::Invalid:
				return TEXT("Invalid");
			case EMigrationResult::FailedFiltering:
				return TEXT("Failed Filtering");
			case EMigrationResult::PassedFiltering:
				return TEXT("Passed Filtering");
			case EMigrationResult::Failed:
				return TEXT("Failed");
			case EMigrationResult::Skipped:
				return TEXT("Skipped");
			case EMigrationResult::Success:
				return TEXT("Success");
		}

		return FString();
	}

private:
	static void OutputReportEntry(const FString& Prefix, const FMigrationRecord* MigrationInfo, FStringBuilderBase& StringBuilder);

	TMap<Worker_EntityId, FEntityMigrationRecord> MigrationResults;
};
