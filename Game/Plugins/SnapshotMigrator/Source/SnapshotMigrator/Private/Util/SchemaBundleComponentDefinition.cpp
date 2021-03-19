#include "SchemaBundleWrappers.h"

SchemaBundleComponentDefinition::SchemaBundleComponentDefinition(const TSharedPtr<FJsonObject>& InComponentDefinition) :
	SchemaBundleDefinitionWithFields(InComponentDefinition->GetArrayField("fields"))
{
	Initialize(InComponentDefinition);
}

SchemaBundleComponentDefinition::SchemaBundleComponentDefinition(const TSharedPtr<FJsonObject>& InComponentDefinition, const SchemaBundleTypeDefinition& InTypeDefinition) :
	SchemaBundleDefinitionWithFields(InTypeDefinition.GetFields())
{
	Initialize(InComponentDefinition);
}

void SchemaBundleComponentDefinition::Initialize(const TSharedPtr<FJsonObject>& ComponentDefinition)
{
	ComponentId = ComponentDefinition->GetIntegerField("componentId");
	SchemaDefinitionQualifiedName = ComponentDefinition->GetStringField("qualifiedName");
	SchemaDefinitionShortName = ComponentDefinition->GetStringField("name");
	ComponentDataDefinition = ExtractDataDefinition(ComponentDefinition);
}

const uint32 SchemaBundleComponentDefinition::GetId() const
{
	return ComponentId;
}

const FString& SchemaBundleComponentDefinition::GetDataDefinition() const
{
	return ComponentDataDefinition;
}

const FString SchemaBundleComponentDefinition::ExtractDataDefinition(const TSharedPtr<FJsonObject>& ComponentDefinition)
{
	return ComponentDefinition->GetStringField("dataDefinition");
}
