#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CWContractTypes.h"
#include "CWTerrainTypes.h"
#include "CWSaveGame.generated.h"

/** Estado guardado de un vehiculo colocado en el nivel. */
USTRUCT()
struct FCWVehicleSaveData
{
	GENERATED_BODY()

	/** Nombre del actor en el nivel (estable para actores colocados). */
	UPROPERTY()
	FString ActorName;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	bool bEngineRunning = false;

	UPROPERTY()
	float FuelLiters = -1.f;

	/** Cuchara (excavadora) o caja (volquete), m3. */
	UPROPERTY()
	float LoadM3 = 0.f;

	UPROPERTY()
	ECWSoilType SoilType = ECWSoilType::Earth;
};

/**
 * Partida guardada (F4.06): contrato, tutorial, economia, jugador, vehiculos y
 * referencia al archivo de terreno guardado junto a ella.
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = CurrentVersion;

	UPROPERTY()
	FString MapName;

	UPROPERTY()
	FDateTime SavedAt;

	UPROPERTY()
	FString ContractId;

	UPROPERTY()
	FCWContractProgress Progress;

	UPROPERTY()
	FCWContractResult LastResult;

	UPROPERTY()
	ECWTutorialStep TutorialStep = ECWTutorialStep::Briefing;

	UPROPERTY()
	float Balance = 0.f;

	UPROPERTY()
	float FuelBaselineLiters = -1.f;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	TArray<FCWVehicleSaveData> Vehicles;

	/** Ruta absoluta del archivo binario de terreno (.cwt). */
	UPROPERTY()
	FString TerrainFile;
};
