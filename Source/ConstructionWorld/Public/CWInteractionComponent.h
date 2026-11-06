#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CWInteractionComponent.generated.h"

class AActor;

/**
 * Componente que traza desde la vista del propietario y detecta actores
 * que implementan ICWInteractable. El HUD consulta GetFocusPrompt().
 */
UCLASS(ClassGroup = (CW), meta = (BlueprintSpawnableComponent))
class CONSTRUCTIONWORLD_API UCWInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "CW|Interaction")
	void Interact();

	UFUNCTION(BlueprintPure, Category = "CW|Interaction")
	AActor* GetFocusedActor() const { return FocusedActor; }

	UFUNCTION(BlueprintPure, Category = "CW|Interaction")
	FText GetFocusPrompt() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Interaction", meta = (ClampMin = "50.0"))
	float TraceDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Interaction", meta = (ClampMin = "1.0"))
	float TraceRadius = 20.f;

protected:
	void UpdateFocus();

	UPROPERTY(BlueprintReadOnly, Category = "CW|Interaction")
	TObjectPtr<AActor> FocusedActor;
};
