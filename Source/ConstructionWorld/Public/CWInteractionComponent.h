#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CWInteractionComponent.generated.h"

class AActor;

/**
 * Componente que busca, desde la vista del propietario, actores que implementan
 * ICWInteractable y enfoca el mejor segun prioridad, distancia maxima propia, cono de mira
 * y linea de vision (F1.03). El HUD consulta GetFocusPrompt().
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

	/** Angulo maximo (grados) entre la mira y el interactuable para poder enfocarlo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Interaction", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float MaxAimAngleDegrees = 35.f;

protected:
	void UpdateFocus();

	UPROPERTY(BlueprintReadOnly, Category = "CW|Interaction")
	TObjectPtr<AActor> FocusedActor;
};
