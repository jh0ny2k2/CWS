#pragma once

#include "CoreMinimal.h"
#include "CWVehicleBase.h"
#include "CWDumpTruck.generated.h"

class ACWDumpZone;
class USceneComponent;
class UStaticMeshComponent;
class UInputAction;
struct FInputActionValue;

/**
 * Camion volquete (Hito H1-B, F2.09-F2.13).
 *
 * - F2.09: rig con chasis, cuatro ruedas y caja basculante con pivote trasero.
 * - F2.10: conduccion con modelo de ruedas por traza (agarre, pendiente y peso).
 * - F2.11: elevacion/retorno de la caja con limites y bloqueo de seguridad.
 * - F2.12: carga abstracta en m3 con indicador de llenado y peso que afecta a la conduccion.
 * - F2.13: descarga validada contra una ACWDumpZone.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWDumpTruck : public ACWVehicleBase
{
	GENERATED_BODY()

public:
	ACWDumpTruck();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Carga (F2.12) -------------------------------------------------------

	/** Anade material hasta llenar la caja. Devuelve los m3 aceptados. */
	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Load")
	float AddLoad(float CubicMeters);

	/** Retira material de la caja. Devuelve los m3 retirados. */
	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Load")
	float RemoveLoad(float CubicMeters);

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	float GetLoadCubicMeters() const { return LoadCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	float GetCapacityCubicMeters() const { return CapacityCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	float GetFillRatio() const
	{
		return CapacityCubicMeters > 0.f ? LoadCubicMeters / CapacityCubicMeters : 0.f;
	}

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	float GetLoadWeightKg() const { return LoadCubicMeters * MaterialDensityKgPerCubicMeter; }

	/** True si la caja esta bajada y aun cabe material. */
	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	bool CanAcceptLoad() const { return CurrentBedAngle < 5.f && LoadCubicMeters < CapacityCubicMeters; }

	// --- Caja basculante (F2.11) --------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Bed")
	void ToggleBed();

	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Bed")
	void SetBedTarget(float AngleDegrees);

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Bed")
	float GetBedAngle() const { return CurrentBedAngle; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Bed")
	float GetBedTarget() const { return TargetBedAngle; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Bed")
	bool IsBedRaisedForDump() const { return CurrentBedAngle >= MinDumpAngle; }

	/** Fija el angulo de la caja de forma inmediata (para pruebas). */
	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Bed")
	void SetBedAngleForTest(float AngleDegrees);

	// --- Descarga (F2.13) ----------------------------------------------------

	/** Descarga toda la carga si la caja esta levantada, el camion parado y
	 *  dentro de una zona de descarga. */
	UFUNCTION(BlueprintCallable, Category = "CW|DumpTruck|Dump")
	bool TryDump();

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Dump")
	bool IsInsideDumpZone() const;

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Dump")
	ACWDumpZone* FindDumpZone() const;

	/** Centro de la caja, usado por la excavadora para descargar material. */
	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Load")
	FVector GetBedCenterLocation() const;

	// --- Estado de conduccion ------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Drive")
	float GetSpeedCmS() const { return CurrentSpeedCmS; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Drive")
	float GetWheelSpinDegrees() const { return WheelSpinDeg; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Drive")
	float GetSteerAngleDegrees() const { return SteerAngle; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpTruck|Drive")
	int32 GetGroundedWheelCount() const { return GroundedWheelCount; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Load")
	float CapacityCubicMeters = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Load")
	float MaterialDensityKgPerCubicMeter = 1600.f;

	/** Perdida de velocidad maxima y aceleracion con la caja llena (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Load",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LoadSpeedPenalty = 0.45f;

	// --- Conduccion (F2.10) --------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Drive")
	float AccelerationCmS2 = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Drive")
	float WheelRadiusCm = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Drive")
	float MaxSteerAngle = 30.f;

	/** Velocidad maxima a la que se permite volcar la caja (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Bed")
	float MaxDumpSpeedCmS = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Bed")
	float MaxBedAngle = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Bed")
	float BedSpeed = 20.f; // deg/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpTruck|Bed")
	float MinDumpAngle = 35.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Bed")
	TObjectPtr<USceneComponent> BedPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Bed")
	TObjectPtr<UStaticMeshComponent> BedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Wheels")
	TObjectPtr<UStaticMeshComponent> WheelFrontLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Wheels")
	TObjectPtr<UStaticMeshComponent> WheelFrontRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Wheels")
	TObjectPtr<UStaticMeshComponent> WheelRearLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Wheels")
	TObjectPtr<UStaticMeshComponent> WheelRearRight;

protected:
	void BedInput(const FInputActionValue& Value);
	void DumpInput(const FInputActionValue& Value);

	void UpdateBed(float DeltaSeconds);
	void UpdateWheels(float DeltaSeconds);
	float GetTractionAndNormal(FVector& OutGroundNormal);

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BedAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DumpAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Load")
	float LoadCubicMeters = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Bed")
	float CurrentBedAngle = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpTruck|Bed")
	float TargetBedAngle = 0.f;

	float CurrentSpeedCmS = 0.f;
	float WheelSpinDeg = 0.f;
	float SteerAngle = 0.f;
	int32 GroundedWheelCount = 0;
};
