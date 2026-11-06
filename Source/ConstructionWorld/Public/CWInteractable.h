#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CWInteractable.generated.h"

/**
 * Interfaz para cualquier actor con el que el jugador pueda interactuar
 * (vehiculos, puertas, paneles, herramientas, puntos de carga...).
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
};
