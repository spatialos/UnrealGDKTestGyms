#include "SchemaBundleWrappers.h"

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

SchemaBundleDefinitions::SchemaBundleDefinitions(const TSharedPtr<FJsonObject>& InSchemaBundleJson)
{
	// We need to do two passes; one to resolve all of our types and a second to resolve our components (including embedded type info on data components)
	for (const TSharedPtr<FJsonValue>& File : InSchemaBundleJson->GetArrayField("schemaFiles"))
	{
		for (const TSharedPtr<FJsonValue>& Type : File->AsObject()->GetArrayField("types"))
		{
			SchemaBundleTypeDefinition T{ Type->AsObject() };
			SchemaTypes.Add(T.GetName(), T);
		}
	}

	for (const TSharedPtr<FJsonValue>& File : InSchemaBundleJson->GetArrayField("schemaFiles"))
	{
		for (const TSharedPtr<FJsonValue>& Component : File->AsObject()->GetArrayField("components"))
		{
			TSharedPtr<FJsonObject> AsObject = Component->AsObject();
			const FString DataDefinition = SchemaBundleComponentDefinition::ExtractDataDefinition(AsObject);
			if (!DataDefinition.IsEmpty())
			{
				SchemaBundleComponentDefinition ComponentDefinition{ AsObject, SchemaTypes.FindChecked(DataDefinition) };
				SchemaComponents.Add(ComponentDefinition.GetId(), ComponentDefinition.GetName(), ComponentDefinition);
			}
			else
			{
				SchemaBundleComponentDefinition ComponentDefinition{ AsObject };
				SchemaComponents.Add(ComponentDefinition.GetId(), ComponentDefinition.GetName(), ComponentDefinition);
			}
		}
	}
}

const SchemaBundleComponentDefinition* SchemaBundleDefinitions::FindComponent(const uint32 Id) const
{
	return SchemaComponents.Find(Id);
}

const SchemaBundleComponentDefinition& SchemaBundleDefinitions::FindComponentChecked(const uint32 Id) const
{
	return SchemaComponents.FindChecked(Id);
}

const SchemaBundleComponentDefinition* SchemaBundleDefinitions::FindComponent(const FString& Name) const
{
	return SchemaComponents.Find(Name);
}

const SchemaBundleComponentDefinition& SchemaBundleDefinitions::FindComponentChecked(const FString& Name) const
{
	return SchemaComponents.FindChecked(Name);
}

const SchemaBundleTypeDefinition* SchemaBundleDefinitions::FindType(const FString& Name) const
{
	return SchemaTypes.Find(Name);
}

const SchemaBundleTypeDefinition& SchemaBundleDefinitions::FindTypeChecked(const FString& Name) const
{
	return SchemaTypes.FindChecked(Name);
}

const bool SchemaBundleDefinitions::GetCorrespondingComponentId(const SchemaBundleDefinitions& FromSchemaBundleDefinitions, const SchemaBundleDefinitions& ToSchemaBundleDefinitions, const uint32 FromComponentId, uint32& ToComponentId)
{
	if (const SchemaBundleComponentDefinition* FromComponentDefinition = FromSchemaBundleDefinitions.FindComponent(FromComponentId))
	{
		if (const SchemaBundleComponentDefinition* ToComponentDefinition = ToSchemaBundleDefinitions.FindComponent(FromComponentDefinition->GetName()))
		{
			ToComponentId = ToComponentDefinition->GetId();
			return true;
		}
	}

	return false;
}
