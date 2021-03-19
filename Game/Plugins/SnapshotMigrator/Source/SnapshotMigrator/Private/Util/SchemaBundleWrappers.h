// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once
#include "SnapshotMigratorModuleInternal.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

template <class TValue>
class SchemaBundleSearchableContainer
{
public:
	const TValue& Add(const uint32 Id, const FString& Name, const TValue& Value)
	{
		int32 Idx = Values.Add(Value);
		IdsToValues.Emplace(Id, Idx);
		NamesToValues.Emplace(Name, Idx);
		check(Values.Num() == IdsToValues.Num() && IdsToValues.Num() == NamesToValues.Num());

		return Values[Idx];
	}

	const TValue* Find(const uint32 Id) const
	{
		if (const int32* IndexPtr = IdsToValues.Find(Id))
		{
			return &Values[*IndexPtr];
		}
		return nullptr;
	}

	const TValue& FindChecked(const uint32 Id) const
	{
		return Values[IdsToValues.FindChecked(Id)];
	}

	const TValue* Find(const FString& Name) const
	{
		if (const int32* IndexPtr = NamesToValues.Find(Name))
		{
			return &Values[*IndexPtr];
		}
		return nullptr;
	}

	const TValue& FindChecked(const FString& Name) const
	{
		return Values[NamesToValues.FindChecked(Name)];
	}

	const TArray<TValue>& GetAll() const
	{
		return Values;
	}

private:
	TArray<TValue> Values;
	TMap<uint32, int32> IdsToValues;
	TMap<FString, int32> NamesToValues;
};

class SchemaBundleFieldDefinition
{
public:
	enum class SchemaPrimitiveType : int
	{
		Invalid = 0,
		Int32 = 1,
		Int64 = 2,
		Uint32 = 3,
		Uint64 = 4,
		Sint32 = 5,
		Sint64 = 6,
		Fixed32 = 7,
		Fixed64 = 8,
		Sfixed32 = 9,
		Sfixed64 = 10,
		Bool = 11,
		Float = 12,
		Double = 13,
		String = 14,
		EntityId = 15,
		Bytes = 16,
		Entity = 17
	};

	enum class TypeIndex : int
	{
		INVALID = -1,
		INNER = 0,
		// Aliased to be the same as INNER; this is purely for readability.
		KEY = 0,
		VALUE = 1
	};

	SchemaBundleFieldDefinition(const TSharedPtr<FJsonObject>& InFieldDefinition);

	// Define this externally so that we can get correct type resolution to use array equality
	friend bool operator==(const SchemaBundleFieldDefinition& LHS, const SchemaBundleFieldDefinition& RHS);

	uint32 GetId() const;

	const FString& GetName() const;

	bool IsSingular() const
	{
		return Definition->HasField(SingularTypeKey);
	}
	bool IsOptional() const
	{
		return Definition->HasField(OptionalTypeKey);
	}
	bool IsList() const
	{
		return Definition->HasField(ListTypeKey);
	}
	bool IsMap() const
	{
		return Definition->HasField(MapTypeKey);
	}

	bool IsPrimitive(const TypeIndex Index) const
	{
		return Types[(int) Index].PrimitiveType > SchemaPrimitiveType::Invalid;
	}
	bool IsEnum(const TypeIndex Index) const
	{
		return Types[(int) Index].bIsEnum;
	};
	bool IsType(const TypeIndex Index) const
	{
		return Types[(int) Index].bIsType;
	};
	bool IsUnrealObjectRefType(const TypeIndex Index) const
	{
		return Types[(int) Index].ResolvedType.Equals(FString{ TEXT("unreal.UnrealObjectRef") });
	};

	bool IsPrimitive() const
	{
		return IsPrimitive(TypeIndex::INNER);
	}
	bool IsEnum() const
	{
		return IsEnum(TypeIndex::INNER);
	}
	bool IsType() const
	{
		return IsType(TypeIndex::INNER);
	}
	bool IsUnrealObjectRefType() const
	{
		return IsUnrealObjectRefType(TypeIndex::INNER);
	}

	const SchemaPrimitiveType GetPrimitiveType(const TypeIndex Index) const
	{
		return Types[(int) Index].PrimitiveType;
	}
	const FString& GetResolvedType(const TypeIndex Index) const
	{
		return Types[(int) Index].ResolvedType;
	};

	const SchemaPrimitiveType GetPrimitiveType() const
	{
		return GetPrimitiveType(TypeIndex::INNER);
	};
	const FString& GetResolvedType() const
	{
		return GetResolvedType(TypeIndex::INNER);
	};

	const bool IsSameTypeAs(const SchemaBundleFieldDefinition& Other) const;

	static const FString COMPONENT_SET_INTEREST_MAP;

private:
	const bool IsSameCardinality(const SchemaBundleFieldDefinition& Other) const;

	TSharedPtr<FJsonObject> Definition;
	uint32 FieldId;
	FString FieldName;

	struct TypeInfo
	{
		SchemaPrimitiveType PrimitiveType = SchemaPrimitiveType::Invalid;
		bool bIsEnum = false;
		bool bIsType = false;

		FString ResolvedType{};

		const bool IsSameTypeAs(const TypeInfo& Other) const
		{
			return PrimitiveType == Other.PrimitiveType && bIsEnum == Other.bIsEnum && bIsType == Other.bIsType && ResolvedType.Equals(Other.ResolvedType);
		}
	};

	TypeInfo Types[2];

	const FString SingularTypeKey{ "singularType" };
	const FString OptionalTypeKey{ "optionType" };
	const FString ListTypeKey{ "listType" };
	const FString MapTypeKey{ "mapType" };

	const TMap<FString, SchemaPrimitiveType> SchemaPrimitiveTypes{
		{ "Int32", SchemaPrimitiveType::Int32 },
		{ "Int64", SchemaPrimitiveType::Int64 },
		{ "Uint32", SchemaPrimitiveType::Uint32 },
		{ "Uint64", SchemaPrimitiveType::Uint64 },
		{ "Sint32", SchemaPrimitiveType::Sint32 },
		{ "Sint64", SchemaPrimitiveType::Sint64 },
		{ "Fixed32", SchemaPrimitiveType::Fixed32 },
		{ "Fixed64", SchemaPrimitiveType::Fixed64 },
		{ "Sfixed32", SchemaPrimitiveType::Sfixed32 },
		{ "Sfixed64", SchemaPrimitiveType::Sfixed64 },
		{ "Bool", SchemaPrimitiveType::Bool },
		{ "Float", SchemaPrimitiveType::Float },
		{ "Double", SchemaPrimitiveType::Double },
		{ "String", SchemaPrimitiveType::String },
		{ "EntityId", SchemaPrimitiveType::EntityId },
		{ "Bytes", SchemaPrimitiveType::Bytes },
		{ "Entity", SchemaPrimitiveType::Entity }
	};
};

class SchemaBundleDefinitionWithFields
{
public:
	enum class NameType : uint8
	{
		QUALIFIED,
		SHORT
	};

	SchemaBundleDefinitionWithFields(const TArray<TSharedPtr<FJsonValue>>& InFields);
	SchemaBundleDefinitionWithFields(const TArray<SchemaBundleFieldDefinition>& InFields);

	virtual ~SchemaBundleDefinitionWithFields()
	{
	}

	const FString& GetName(const NameType Type = NameType::QUALIFIED) const;

	const SchemaBundleFieldDefinition* FindField(const uint32 Id) const;

	const SchemaBundleFieldDefinition& FindFieldChecked(const uint32 Id) const;

	const SchemaBundleFieldDefinition* FindField(const FString& Name) const;

	const SchemaBundleFieldDefinition& FindFieldChecked(const FString& Name) const;

	const TArray<SchemaBundleFieldDefinition>& GetFields() const;

	const TArray<SchemaBundleFieldDefinition>& GetUnrealObjectRefFields() const;

protected:
	FString SchemaDefinitionQualifiedName;
	FString SchemaDefinitionShortName;

private:
	SchemaBundleSearchableContainer<SchemaBundleFieldDefinition> Fields;
	TArray<SchemaBundleFieldDefinition> UnrealObjectRefFields;
};

class SchemaBundleTypeDefinition : public SchemaBundleDefinitionWithFields
{
public:
	SchemaBundleTypeDefinition(const TSharedPtr<FJsonObject>& InTypeDefinition);
};

class SchemaBundleComponentDefinition : public SchemaBundleDefinitionWithFields
{
public:
	SchemaBundleComponentDefinition(const TSharedPtr<FJsonObject>& InComponentDefinition);
	SchemaBundleComponentDefinition(const TSharedPtr<FJsonObject>& InComponentDefinition, const SchemaBundleTypeDefinition& InTypeDefinition);

	const uint32 GetId() const;

	const FString& GetDataDefinition() const;

	static const FString ExtractDataDefinition(const TSharedPtr<FJsonObject>& ComponentDefinition);

private:
	uint32 ComponentId;
	FString ComponentDataDefinition;

	void Initialize(const TSharedPtr<FJsonObject>& ComponentDefinition);
};

class SchemaBundleDefinitions
{
public:
	SchemaBundleDefinitions()
	{
	}

	~SchemaBundleDefinitions()
	{
	}

	SchemaBundleDefinitions(const TSharedPtr<FJsonObject>& InSchemaBundleJson);

	const SchemaBundleComponentDefinition* FindComponent(const uint32 Id) const;

	const SchemaBundleComponentDefinition& FindComponentChecked(const uint32 Id) const;

	const SchemaBundleComponentDefinition* FindComponent(const FString& Name) const;

	const SchemaBundleComponentDefinition& FindComponentChecked(const FString& Name) const;

	const SchemaBundleTypeDefinition* FindType(const FString& Name) const;

	const SchemaBundleTypeDefinition& FindTypeChecked(const FString& Name) const;

	static const bool GetCorrespondingComponentId(const SchemaBundleDefinitions& FromSchemaBundleDefinitions, const SchemaBundleDefinitions& ToSchemaBundleDefinitions, const uint32 FromComponentId, uint32& ToComponentId);

private:
	SchemaBundleSearchableContainer<SchemaBundleComponentDefinition> SchemaComponents;
	// Types are only ever referenced by name, so a simple map suffices.
	TMap<FString, SchemaBundleTypeDefinition> SchemaTypes;
};
