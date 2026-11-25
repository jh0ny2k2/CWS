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

protected:
	/** Panel de instrumentacion de cabina (H4 / F5.06). */
	void DrawCabinInstruments(const class ACWExcavator& Excavator, float CenterX, float CenterY);

	/** Panel de mision (F4.03): contrato, objetivo, excavado, carga y % entregado. */
	void DrawMissionPanel(const class UCWContractSubsystem& Contract);

	/** Marcador del siguiente objetivo en pantalla o flecha en el borde (F4.03). */
	void DrawObjectiveMarker(const class UCWContractSubsystem& Contract);

	/** Avisos del jugador con fundido (F4.04). */
	void DrawNotices();

	void DrawBar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill);
};
