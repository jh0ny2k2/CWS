#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class APawn;
class UInputAction;
class UInputMappingContext;
struct FEnhancedActionKeyMapping;

/**
 * Ajustes y utilidades de entrada compartidas por personaje y maquinaria (F1.02 / F1.05 / F1.07).
 *
 * Consola:
 *   cw.Input.MouseSensitivity 1.0   Multiplicador del raton.
 *   cw.Input.GamepadLookRate 45     Velocidad del stick derecho (unidades/s antes de la escala del motor).
 *   cw.Input.InvertY 0|1            Invierte el eje vertical de la camara.
 */
namespace CWInput
{
	CONSTRUCTIONWORLD_API float GetMouseSensitivity();
	CONSTRUCTIONWORLD_API float GetGamepadLookRate();
	CONSTRUCTIONWORLD_API bool IsLookYInverted();

	/**
	 * Aplica un giro de camara al pawn. Con bIsRate (stick) el valor se escala por DeltaSeconds
	 * para que la velocidad no dependa de los FPS; con raton se aplica la sensibilidad.
	 */
	CONSTRUCTIONWORLD_API void ApplyLook(APawn* Pawn, const FVector2D& Axis, bool bIsRate);

	/** Mapea una tecla a una accion, opcionalmente negada y con zona muerta (sticks). */
	CONSTRUCTIONWORLD_API FEnhancedActionKeyMapping& MapKey(
		UInputMappingContext* Context, UInputAction* Action, const FKey& Key, UObject* Outer,
		bool bNegate = false, bool bDeadZone = false);
}
