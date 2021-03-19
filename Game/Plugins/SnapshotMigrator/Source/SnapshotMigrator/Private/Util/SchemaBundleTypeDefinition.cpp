#include "SchemaBundleWrappers.h"

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

SchemaBundleTypeDefinition::SchemaBundleTypeDefinition(const TSharedPtr<FJsonObject>& InTypeDefinition) :
	SchemaBundleDefinitionWithFields(InTypeDefinition->GetArrayField("fields"))
{
	SchemaDefinitionQualifiedName = InTypeDefinition->GetStringField("qualifiedName");
	SchemaDefinitionShortName = InTypeDefinition->GetStringField("name");
}
