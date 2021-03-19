#include "SchemaBundleWrappers.h"

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

SchemaBundleDefinitionWithFields::SchemaBundleDefinitionWithFields(const TArray<TSharedPtr<FJsonValue>>& InFields)
{
	for (const TSharedPtr<FJsonValue>& Field : InFields)
	{
		const SchemaBundleFieldDefinition F{ Field->AsObject() };
		Fields.Add(F.GetId(), F.GetName(), F);
	}

	UnrealObjectRefFields = Fields.GetAll().FilterByPredicate([](const SchemaBundleFieldDefinition& FieldDefinition) { return FieldDefinition.IsUnrealObjectRefType(); });
}

SchemaBundleDefinitionWithFields::SchemaBundleDefinitionWithFields(const TArray<SchemaBundleFieldDefinition>& InFields)
{
	for (const SchemaBundleFieldDefinition& FieldDefinition : InFields)
	{
		Fields.Add(FieldDefinition.GetId(), FieldDefinition.GetName(), FieldDefinition);
	}

	UnrealObjectRefFields = Fields.GetAll().FilterByPredicate([](const SchemaBundleFieldDefinition& FieldDefinition) { return FieldDefinition.IsUnrealObjectRefType(); });
}

const FString& SchemaBundleDefinitionWithFields::GetName(const NameType Type /* = NameType::QUALIFIED */) const
{
	switch (Type)
	{
		case NameType::SHORT:
			return SchemaDefinitionShortName;
		case NameType::QUALIFIED:
		default:
			return SchemaDefinitionQualifiedName;
	}
}

const SchemaBundleFieldDefinition* SchemaBundleDefinitionWithFields::FindField(const uint32 Id) const
{
	return Fields.Find(Id);
}

const SchemaBundleFieldDefinition& SchemaBundleDefinitionWithFields::FindFieldChecked(const uint32 Id) const
{
	return Fields.FindChecked(Id);
}

const SchemaBundleFieldDefinition* SchemaBundleDefinitionWithFields::FindField(const FString& Name) const
{
	return Fields.Find(Name);
}

const SchemaBundleFieldDefinition& SchemaBundleDefinitionWithFields::FindFieldChecked(const FString& Name) const
{
	return Fields.FindChecked(Name);
}

const TArray<SchemaBundleFieldDefinition>& SchemaBundleDefinitionWithFields::GetFields() const
{
	return Fields.GetAll();
}

const TArray<SchemaBundleFieldDefinition>& SchemaBundleDefinitionWithFields::GetUnrealObjectRefFields() const
{
	return UnrealObjectRefFields;
}
