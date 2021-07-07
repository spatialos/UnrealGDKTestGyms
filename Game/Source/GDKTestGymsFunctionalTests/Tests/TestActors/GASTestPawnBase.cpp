// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "GASTestPawnBase.h"
#include "Net/UnrealNetwork.h"

AGASTestPawnBase::AGASTestPawnBase()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

void AGASTestPawnBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GrantInitialAbilitiesIfNeeded();
	}
}

void AGASTestPawnBase::OnAuthorityGained()
{
	Super::OnAuthorityGained();
	GrantInitialAbilitiesIfNeeded();
}

void AGASTestPawnBase::GrantInitialAbilitiesIfNeeded()
{
	if (!bHasGrantedAbilities)
	{
		for (const TSubclassOf<UGameplayAbility>& Ability : GetInitialGrantedAbilities())
		{
			FGameplayAbilitySpec Spec = FGameplayAbilitySpec(Ability);
			FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
		}

		bHasGrantedAbilities = true;
	}
}

void AGASTestPawnBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AGASTestPawnBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AGASTestPawnBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, bHasGrantedAbilities, COND_ServerOnly);
}
