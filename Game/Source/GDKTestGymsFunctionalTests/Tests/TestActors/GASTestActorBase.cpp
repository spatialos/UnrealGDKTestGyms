// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GASTestActorBase.h"
#include "Net/UnrealNetwork.h"

AGASTestActorBase::AGASTestActorBase()
{
	bReplicates = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

void AGASTestActorBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GrantInitialAbilitiesIfNeeded();
	}
}

void AGASTestActorBase::OnAuthorityGained()
{
	Super::OnAuthorityGained();
	GrantInitialAbilitiesIfNeeded();
}

void AGASTestActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, bHasGrantedAbilities, COND_ServerOnly);
}

void AGASTestActorBase::GrantInitialAbilitiesIfNeeded()
{
	if (!bHasGrantedAbilities)
	{
		for (const TSubclassOf<UGameplayAbility>& Ability : GetInitialGrantedAbilities())
		{
			FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability));
		}

		bHasGrantedAbilities = true;
	}
}
