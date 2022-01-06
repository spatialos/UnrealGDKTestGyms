#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "CounterComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCounterComponent, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDKTESTGYMS_API UCounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCounterComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReportFrequency = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AActor>> ClassesToCount;

	int32 GetActorClassCount(TSubclassOf<AActor> ActorClass) const;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:	

	float Timer;
	TMap<TSubclassOf<AActor>, int32> CachedClassCounts;

	void UpdateCachedActorCounts();
};