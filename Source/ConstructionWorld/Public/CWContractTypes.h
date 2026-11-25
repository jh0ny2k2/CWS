#pragma once

#include "CoreMinimal.h"
#include "CWContractTypes.generated.h"

/**
 * Estados de un contrato (F4.01). Transiciones validas:
 *   Available  -> Accepted
 *   Accepted   -> InProgress | Abandoned
 *   InProgress -> Completed | Failed | Abandoned
 *   Completed / Failed / Abandoned -> Available (reiniciar)
 */
UENUM(BlueprintType)
enum class ECWContractState : uint8
{
	Available  UMETA(DisplayName = "Disponible"),
	Accepted   UMETA(DisplayName = "Aceptado"),
	InProgress UMETA(DisplayName = "En curso"),
	Completed  UMETA(DisplayName = "Completado"),
	Failed     UMETA(DisplayName = "Fallido"),
	Abandoned  UMETA(DisplayName = "Abandonado")
};

/** Pasos del tutorial de CWS-001 (F4.04). Se avanzan solos al cumplir la condicion. */
UENUM(BlueprintType)
enum class ECWTutorialStep : uint8
{
	Briefing,
	GoToExcavator,
	EnterExcavator,
	StartEngine,
	DigMode,
	DigInParcel,
	LoadTruck,
	EnterTruck,
	DriveToZone,
	Dump,
	Finished
};

/** Definicion fija de un contrato: objetivo, pagos y costes (F4.05). */
USTRUCT(BlueprintType)
struct FCWContractDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	FString Id = TEXT("CWS-001");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	FString Title = TEXT("Vaciado de parcela");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	FString Client = TEXT("Promociones Rivera S.L.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	FString Description = TEXT("Excava la parcela balizada y lleva la tierra en el volquete hasta la zona de descarga.");

	/** Volumen que hay que entregar en la zona de descarga (m3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract", meta = (ClampMin = "0.1"))
	float TargetVolumeM3 = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	float Reward = 4500.f;

	/** Limite de tiempo en segundos de juego; 0 = sin limite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract")
	float TimeLimitSeconds = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract|Costs")
	float FuelPricePerLiter = 1.45f;

	/** Coste por hora de juego de excavadora + volquete (alquiler y operario). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract|Costs")
	float MachineHourlyRate = 165.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract|Costs")
	float OutsideDigPenaltyPerM3 = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Contract|Costs")
	float IllegalDumpPenaltyPerM3 = 25.f;
};

/** Progreso medido del contrato. */
USTRUCT(BlueprintType)
struct FCWContractProgress
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	ECWContractState State = ECWContractState::Available;

	/** Excavado neto dentro de la parcela (lo devuelto a la parcela se resta). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float ExcavatedM3 = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float DeliveredM3 = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float OutsideDigM3 = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float IllegalDumpM3 = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float ElapsedSeconds = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float FuelUsedLiters = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	int32 Trips = 0;
};

/** Liquidacion al cerrar el contrato. */
USTRUCT(BlueprintType)
struct FCWContractResult
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	bool bValid = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float Reward = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float FuelCost = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float MachineCost = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float Penalties = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float Net = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "CW|Contract")
	float ElapsedSeconds = 0.f;
};

namespace CWContract
{
	CONSTRUCTIONWORLD_API const TCHAR* StateName(ECWContractState State);
	CONSTRUCTIONWORLD_API bool IsTransitionAllowed(ECWContractState From, ECWContractState To);
	CONSTRUCTIONWORLD_API FString FormatMoney(float Euros);
	CONSTRUCTIONWORLD_API FString FormatDuration(float Seconds);
}
