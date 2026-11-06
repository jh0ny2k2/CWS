#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWHUD.generated.h"

/**
 * HUD provisional de prototipo: punto de mira y prompt de interaccion.
 * Se sustituira por UMG en la fase de interfaz.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
