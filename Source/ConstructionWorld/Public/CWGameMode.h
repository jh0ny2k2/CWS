#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CWGameMode.generated.h"

/**
 * GameMode base del proyecto. De momento solo define el pawn por defecto.
 * Aqui iran mas adelante: reglas de contrato, economia y progresion.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACWGameMode();
};
