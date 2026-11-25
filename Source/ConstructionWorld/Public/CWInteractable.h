#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CWInteractable.generated.h"

/**
 * Interfaz para cualquier actor con el que el jugador pueda interactuar
 * (vehiculos, puertas, paneles, herramientas, puntos de carga...).
 *
 * F1.03: ademas del prompt, cada actor puede declarar prioridad, distancia maxima propia y
 * si acepta la interaccion ahora mismo. UCWInteractionComponent elige el mejor candidato.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCWInteractable : public UInterface
{
	GENERATED_BODY()
};

class CONSTRUCTIONWORLD_API ICWInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CW|Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CW|Interaction")
	FText GetInteractionPrompt() const;

	/** Mayor valor gana cuando varios interactuables estan a tiro (p. ej. puerta > vehiculo). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CW|Interaction")
	int32 GetInteractionPriority() const;

	/** Distancia maxima (cm) desde la vista del jugador. <= 0 usa la del componente. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CW|Interaction")
	float GetMaxInteractionDistance() const;

	/** Si es false el actor no se enfoca (p. ej. vehiculo ya ocupado por otro). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CW|Interaction")
	bool CanInteract(AActor* Interactor) const;

	virtual int32 GetInteractionPriority_Implementation() const { return 0; }
	virtual float GetMaxInteractionDistance_Implementation() const { return 0.f; }
	virtual bool CanInteract_Implementation(AActor* Interactor) const { return true; }
};
