#pragma once

#include "CoreMinimal.h"
#include "CWTerrainTypes.generated.h"

/** Tipos de suelo del prototipo de terreno (H2 / Fase 4). */
UENUM(BlueprintType)
enum class ECWSoilType : uint8
{
	Earth  UMETA(DisplayName = "Tierra"),
	Gravel UMETA(DisplayName = "Grava"),
	Sand   UMETA(DisplayName = "Arena"),
	Clay   UMETA(DisplayName = "Arcilla")
};

/** Propiedades fisicas de cada tipo de suelo. */
USTRUCT(BlueprintType)
struct FCWSoilProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Soil")
	ECWSoilType Type = ECWSoilType::Earth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Soil")
	FString DisplayName = TEXT("Tierra");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Soil")
	float DensityKgPerCubicMeter = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Soil")
	float AngleOfReposeDegrees = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Soil")
	FLinearColor Color = FLinearColor(0.34f, 0.24f, 0.14f);
};

namespace CWSoil
{
	/** Devuelve las propiedades del tipo de suelo indicado. */
	CONSTRUCTIONWORLD_API FCWSoilProperties GetSoilProperties(ECWSoilType Type);

	/** Densidad en kg/m3 del tipo de suelo. */
	CONSTRUCTIONWORLD_API float GetDensityKgPerCubicMeter(ECWSoilType Type);
}
