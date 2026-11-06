#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWHUD.generated.h"

/**
 * HUD provisional de prototipo: punto de mira, prompt de interaccion y modo de control.
 *
 * BUG-008 corregido: cuando el pawn es un vehiculo (ACWVehicleBase) se obtiene el
 * InteractionComponent del personaje conductor en lugar del pawn activo, que no lo tiene.
 * Ademas se muestra una linea de ayuda con los controles del vehiculo.
 *
 * Se sustituira por UMG en la fase de interfaz (H3).
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
