// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SnapshotMigratorReport.h"

#include "SnapshotMigratorModule.h"
#include "SpatialConstants.h"

void FMigrationRecord::ToString(FStringBuilderBase& StringBuilder) const
{
	StringBuilder.Append(FString::Printf(TEXT("(%s)"), *SnapshotMigratorReport::MigrationResultToString(Result)));

	if (!ResultReason.IsEmpty())
	{
		StringBuilder.Append(FString::Printf(TEXT(", Reason: %s"), *ResultReason));
	}
}

void FEntityMigrationRecord::ToString(FStringBuilderBase& StringBuilder) const
{
	StringBuilder.Append(FString::Printf(TEXT("EntityId: %lld, Classification: %s "), SourceEntityId, *SnapshotMigratorReport::EntityClassificationToString(Classification)));
	FMigrationRecord::ToString(StringBuilder);
}

void FComponentMigrationRecord::ToString(FStringBuilderBase& StringBuilder) const
{
	StringBuilder.Append(FString::Printf(TEXT("ComponentId: %d -> %d "), SourceComponentId, TargetComponentId));
	FMigrationRecord::ToString(StringBuilder);
}

void FFieldMigrationRecord::ToString(FStringBuilderBase& StringBuilder) const
{
	StringBuilder.Append(FString::Printf(TEXT("FieldId: %d -> %d, Name: %s "), SourceFieldId, TargetFieldId, *FieldName));
	FMigrationRecord::ToString(StringBuilder);
}

SnapshotMigratorReport::SnapshotMigratorReport(const int32 EntityCount)
{
	MigrationResults.Reserve(EntityCount);
}

FEntityMigrationRecord& SnapshotMigratorReport::AddEntityMigrationRecord(const Worker_EntityId EntityId)
{
	return MigrationResults.Add(EntityId);
}

FEntityMigrationRecord& SnapshotMigratorReport::GetEntityMigrationRecord(const Worker_EntityId EntityId)
{
	return MigrationResults.FindChecked(EntityId);
}

void SnapshotMigratorReport::OutputReportToLog()
{
	TArray<Worker_EntityId> EntityIds;
	MigrationResults.GetKeys(EntityIds);
	EntityIds.Sort();

	TArray<EEntityClassification> Classifications{ EEntityClassification::System, EEntityClassification::Dynamic, EEntityClassification::Startup };

	UE_LOG(LogSnapshotMigration, Log, TEXT("-------------- Snapshot Migration Summary --------------"));

	// TODO: do this in one pass over the entities
	for (auto Classification : Classifications)
	{
		UE_LOG(LogSnapshotMigration, Log, TEXT("Classification: %s"), *SnapshotMigratorReport::EntityClassificationToString(Classification));

		TMap<EMigrationResult, uint32> EntitySummary;

		for (Worker_EntityId EntityId : EntityIds)
		{
			if (FEntityMigrationRecord* EntityMigrationRecord = MigrationResults.Find(EntityId))
			{
				if (EntityMigrationRecord->Classification == Classification)
				{
					uint32& ClassificationMigrationResultCount = EntitySummary.FindOrAdd(EntityMigrationRecord->Result);
					ClassificationMigrationResultCount++;
				}
			}
		}

		TArray<EMigrationResult> KeyArray;
		EntitySummary.GetKeys(KeyArray);

		for (EMigrationResult MigrationResult : KeyArray)
		{
			UE_LOG(LogSnapshotMigration, Log, TEXT("%s %d"),
				*SnapshotMigratorReport::MigrationResultToString(MigrationResult),
				EntitySummary[MigrationResult]);
		}
	}

	UE_LOG(LogSnapshotMigration, Log, TEXT("-------------- End Snapshot Migration Summary --------------"));

	static const TArray<Worker_ComponentId> SuppressOutputForComponents = {
		SpatialConstants::MULTICAST_RPCS_COMPONENT_ID,
		SpatialConstants::SERVER_ENDPOINT_COMPONENT_ID,
		SpatialConstants::CLIENT_ENDPOINT_COMPONENT_ID
	};

	TStringBuilder<256> StringBuilder;
	for (const Worker_EntityId EntityId : EntityIds)
	{
		if (FEntityMigrationRecord* EntityMigrationRecord = MigrationResults.Find(EntityId))
		{
			OutputReportEntry(TEXT(""), EntityMigrationRecord, StringBuilder);

			EntityMigrationRecord->ComponentMigrationRecords.Sort();
			for (FComponentMigrationRecord& ComponentMigrationRecord : EntityMigrationRecord->ComponentMigrationRecords)
			{
				if (SuppressOutputForComponents.Contains(ComponentMigrationRecord.TargetComponentId))
				{
					continue;
				}

				OutputReportEntry(TEXT(" - "), &ComponentMigrationRecord, StringBuilder);

				ComponentMigrationRecord.FieldMigrationRecords.Sort();
				for (const FFieldMigrationRecord& FieldMigrationRecord : ComponentMigrationRecord.FieldMigrationRecords)
				{
					OutputReportEntry(TEXT(" -- "), &FieldMigrationRecord, StringBuilder);
				}
			}
		}
	}
}

void SnapshotMigratorReport::OutputReportEntry(const FString& Prefix, const FMigrationRecord* MigrationInfo, FStringBuilderBase& StringBuilder)
{
	StringBuilder.Reset();
	StringBuilder.Append(Prefix);
	MigrationInfo->ToString(StringBuilder);

	UE_LOG(LogSnapshotMigration, Log, TEXT("%s"), StringBuilder.ToString());
}
