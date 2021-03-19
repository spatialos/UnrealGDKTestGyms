#include "SchemaBundleWrappers.h"
#include "Util/SnapshotHelperLibrary.h"

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// This type doesn't actually exist in schema. It exists so we can treat ComponentInterestMap
// fields like regular object fields with named types when migrating the field.
// See SnapshotDataMigrator::MigrateObjectField.
const FString SchemaBundleFieldDefinition::COMPONENT_SET_INTEREST_MAP{ TEXT("snapshotmigrator.ComponentSetInterestMap") };

SchemaBundleFieldDefinition::SchemaBundleFieldDefinition(const TSharedPtr<FJsonObject>& InFieldDefinition) :
	Definition(InFieldDefinition)
{
	FieldId = InFieldDefinition->GetIntegerField("fieldId");
	FieldName = InFieldDefinition->GetStringField("name");

	auto ProcessValueType = [this](const TSharedPtr<FJsonObject>& TypeReference, const TypeIndex Index = TypeIndex::INNER) {
		int IntIndex = static_cast<int>(Index);
		if (TypeReference->HasField("primitive"))
		{
			Types[IntIndex].PrimitiveType = SchemaPrimitiveTypes.FindChecked(TypeReference->GetStringField("primitive"));
		}
		else if (TypeReference->HasField("enum"))
		{
			Types[IntIndex].bIsEnum = true;
			Types[IntIndex].ResolvedType = TypeReference->GetStringField("enum");
		}
		else if (TypeReference->HasField("type"))
		{
			Types[IntIndex].bIsType = true;
			Types[IntIndex].ResolvedType = TypeReference->GetStringField("type");
		}
		else
		{
			checkNoEntry();
		}
	};

	if (IsSingular())
	{
		ProcessValueType(Definition->GetObjectField(SingularTypeKey)->GetObjectField("type"));
	}
	else if (IsOptional())
	{
		ProcessValueType(Definition->GetObjectField(OptionalTypeKey)->GetObjectField("innerType"));
	}
	else if (IsList())
	{
		ProcessValueType(Definition->GetObjectField(ListTypeKey)->GetObjectField("innerType"));
	}
	else if (IsMap())
	{
		TSharedPtr<FJsonObject> MapType = Definition->GetObjectField(MapTypeKey);
		ProcessValueType(MapType->GetObjectField("keyType"), TypeIndex::KEY);
		ProcessValueType(MapType->GetObjectField("valueType"), TypeIndex::VALUE);
	}
	else
	{
		checkNoEntry();
	}
}

bool operator==(const SchemaBundleFieldDefinition& LHS, const SchemaBundleFieldDefinition& RHS)
{
	return LHS.FieldName == RHS.FieldName && LHS.IsSameTypeAs(RHS);
}

uint32 SchemaBundleFieldDefinition::GetId() const
{
	return FieldId;
}

const FString& SchemaBundleFieldDefinition::GetName() const
{
	return FieldName;
}

const bool SchemaBundleFieldDefinition::IsSameTypeAs(const SchemaBundleFieldDefinition& Other) const
{
	return
		// Same cardinality (singular, optional, list, map)
		IsSameCardinality(Other) &&
		// Same first type (for singular, optional, and list, this is the only type. For maps, it's the key type)
		Types[(int) TypeIndex::INNER].IsSameTypeAs(Other.Types[(int) TypeIndex::INNER]) &&
		// Either we're not a map and the above check is sufficient (and we already know the other isn't a map either, in this case)
		// OR we are a map and we need to make sure our value types match as well.
		(!IsMap() || Types[(int) TypeIndex::VALUE].IsSameTypeAs(Other.Types[(int) TypeIndex::VALUE]));
}

const bool SchemaBundleFieldDefinition::IsSameCardinality(const SchemaBundleFieldDefinition& Other) const
{
	return IsSingular() == Other.IsSingular() &&
		   IsOptional() == Other.IsOptional() &&
		   IsList() == Other.IsList() &&
		   IsMap() == Other.IsMap();
}
