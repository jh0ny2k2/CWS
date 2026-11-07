#pragma once

#include "CoreMinimal.h"
#include "CWVehicleBase.h"
#include "CWExcavator.generated.h"

class ACWDumpTruck;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class USceneComponent;
class UAudioComponent;
class UInputAction;
class UInputMappingContext;
class USoundBase;
struct FInputActionValue;

/** Modo de control activo para la excavadora: orugas (traslado) o brazo hidraulico (excavacion). */
UENUM(BlueprintType)
enum class ECWExcavatorControlMode : uint8
{
	Drive UMETA(DisplayName = "Conduccion (Orugas)"),
	Dig   UMETA(DisplayName = "Excavacion (Brazo e Hidraulica)")
};

/**
 * Excavadora de cadenas interactiva (Hito H1-B).
 *
 * Jerarquia cinematica articulada:
 *   Root (CollisionBox) -> ChassisMesh (Orugas)
 *     -> TurretMount (Torreta giratoria)
 *       -> TurretMesh (Cabina/Torreta)
 *       -> CabinCamera (Vista interior)
 *       -> BoomPivot (Pluma principal)
 *         -> BoomMesh
 *         -> ArmPivot (Balancin/Brazo secundario)
 *           -> ArmMesh
 *           -> BucketPivot (Cuchara)
 *             -> BucketMesh
 *             -> BucketTip (Punto de excavacion)
 *             -> BucketCamera (Camara de precision)
 *
 * Tareas H1-B cubiertas:
 *   - F2.01: Geometria jerarquica con pivotes articulados y escala coherente.
 *   - F2.02: Rotacion de torreta continua o limitada (bContinuousTurret).
 *   - F2.03: Pluma, brazo y cuchara con limites angulares y velocidades configurables.
 *   - F2.04: Cilindros hidraulicos colocados entre anclajes reales cada frame.
 *   - F2.05: Orugas izquierda/derecha con giro estacionario y agarre por traza.
 *   - F2.06: Multiples camaras (exterior, cabina y enfoque a cuchara con tecla C).
 *   - F2.07: Arranque/parada, combustible simple y audio de motor/hidraulica opcional.
 *   - F2.08: Modelo de estabilidad estatica (margen frente a vuelco) para pruebas.
 *   - F2.12: Cuchara con carga abstracta que puede transferirse al camion.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWExcavator : public ACWVehicleBase
{
	GENERATED_BODY()

public:
	ACWExcavator();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Alterna entre modo de traslado (orugas) y modo de excavacion (brazo hidraulico). Tecla R. */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator")
	void ToggleControlMode();

	/** Cicla entre camaras: Exterior -> Cabina -> Cuchara. Tecla C. */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator")
	void CycleCamera();

	// Getters de estado angular actual
	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	float GetTurretYaw() const { return CurrentTurretYaw; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	float GetBoomPitch() const { return CurrentBoomPitch; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	float GetArmPitch() const { return CurrentArmPitch; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	float GetBucketPitch() const { return CurrentBucketPitch; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	ECWExcavatorControlMode GetControlMode() const { return ControlMode; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	FVector GetBucketTipLocation() const;

	UFUNCTION(BlueprintPure, Category = "CW|Excavator")
	int32 GetActiveCameraIndex() const { return ActiveCameraIndex; }

	// --- F2.07: motor y combustible -----------------------------------------

	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Engine")
	void StartEngine();

	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Engine")
	void StopEngine();

	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Engine")
	void ToggleEngine();

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Engine")
	bool IsEngineRunning() const { return bEngineRunning; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Engine")
	float GetFuelLiters() const { return FuelLiters; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Engine")
	float GetFuelRatio() const { return MaxFuelLiters > 0.f ? FuelLiters / MaxFuelLiters : 0.f; }

	/** Anade combustible (litros). Devuelve lo realmente anadido. */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Engine")
	float Refuel(float Liters);

	// --- F2.05: orugas -------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Tracks")
	float GetLeftTrackSpeed() const { return LeftTrackSpeed; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Tracks")
	float GetRightTrackSpeed() const { return RightTrackSpeed; }

	// --- F2.12: carga de cuchara --------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Load")
	float GetBucketLoadCubicMeters() const { return BucketLoadCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Load")
	float GetBucketCapacityCubicMeters() const { return BucketCapacityCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|Excavator|Load")
	float GetBucketFillRatio() const
	{
		return BucketCapacityCubicMeters > 0.f ? BucketLoadCubicMeters / BucketCapacityCubicMeters : 0.f;
	}

	/** Excava si la cuchara esta vacia y toca el suelo; si esta llena y hay un
	 *  camion cerca, descarga en el. Tecla T. */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Load")
	bool DigOrLoadTruck();

	/** Fija la carga de la cuchara (para pruebas y depuracion). */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Load")
	void SetBucketLoadForTest(float CubicMeters);

	// --- F2.08: estabilidad --------------------------------------------------

	/** Margen de vuelco en cm: distancia del centro de masas proyectado al borde
	 *  de la poligonal de apoyo. Negativo o menor que SafetyMarginCm = riesgo. */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Stability")
	float GetStabilityMarginCm(float SlopeDegrees = 0.f, float SlopeDirectionDegrees = 0.f) const;

	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Stability")
	bool IsStableOnSlope(float SlopeDegrees, float SlopeDirectionDegrees) const;

	/** Coloca la articulacion en una pose concreta (para pruebas). */
	UFUNCTION(BlueprintCallable, Category = "CW|Excavator|Stability")
	void SetTestPose(float TurretYaw, float BoomPitch, float ArmPitch, float BucketPitch);

	// Limites y velocidades (grados)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float TurretSpeed = 45.f; // deg/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	bool bContinuousTurret = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MinTurretYaw = -135.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MaxTurretYaw = 135.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float BoomSpeed = 30.f; // deg/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MinBoomPitch = -25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MaxBoomPitch = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float ArmSpeed = 35.f; // deg/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MinArmPitch = -40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MaxArmPitch = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float BucketSpeed = 50.f; // deg/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MinBucketPitch = -55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float MaxBucketPitch = 85.f;

	// --- Motor (F2.07) -------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Engine")
	float MaxFuelLiters = 120.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Engine")
	float FuelLiters = 120.f;

	/** Consumo a plena carga en litros por hora. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Engine")
	float FuelConsumptionLitersPerHour = 6.f;

	/** Sonido de motor opcional; si esta vacio se usa solo el log (placeholder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Engine")
	TObjectPtr<USoundBase> EngineSound;

	/** Sonido hidraulico opcional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Engine")
	TObjectPtr<USoundBase> HydraulicSound;

	// --- Carga (F2.12) -------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Load")
	float BucketCapacityCubicMeters = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Load")
	float MaterialDensityKgPerCubicMeter = 1600.f;

	/** Distancia maxima de la punta de cuchara al suelo para excavar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Load")
	float DigReachCm = 220.f;

	/** Distancia maxima al camion para descargar la cuchara. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Load")
	float TruckLoadReachCm = 800.f;

	// --- Estabilidad (F2.08) -------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float ChassisMassKg = 12000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float TurretMassKg = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float BoomMassKg = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float ArmMassKg = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float BucketMassKg = 500.f;

	/** Semilongitud de la poligonal de apoyo (orugas) en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float TrackHalfLengthCm = 300.f;

	/** Semianchura de la poligonal de apoyo (orugas) en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float TrackHalfWidthCm = 150.f;

	/** Margen minimo de seguridad antes de considerar riesgo de vuelco. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Stability")
	float SafetyMarginCm = 20.f;

protected:
	virtual void EnsureInputObjects();

	// Handlers de entrada
	void DigBoomInput(const FInputActionValue& Value);
	void DigArmInput(const FInputActionValue& Value);
	void DigTurretInput(const FInputActionValue& Value);
	void DigBucketInput(const FInputActionValue& Value);
	void ToggleModeInput(const FInputActionValue& Value);
	void CycleCameraInput(const FInputActionValue& Value);
	void EngineInput(const FInputActionValue& Value);
	void DigCycleInput(const FInputActionValue& Value);

	void UpdateEngine(float DeltaSeconds);
	void UpdateTracks(float DeltaSeconds);
	void UpdateHydraulics();
	void UpdateBucketLoadVisual();

	/** Coloca un cilindro entre dos puntos del mundo, escalado a la longitud. */
	void PlaceCylinder(UStaticMeshComponent* Cylinder, const FVector& WorldA, const FVector& WorldB,
		float RadiusScale) const;

	/** Busca un camion volquete al alcance de la cuchara. */
	ACWDumpTruck* FindNearbyTruck() const;

	/** Comprueba si la punta de la cuchara esta cerca del suelo. */
	bool IsBucketTipNearGround() const;

	/** Aplica el estado de los angulos a los componentes. */
	void ApplyJointRotations();

	// Componentes visuales y articulares
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<USceneComponent> TurretMount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<UStaticMeshComponent> TurretMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<USceneComponent> BoomPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<UStaticMeshComponent> BoomMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<USceneComponent> ArmPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<UStaticMeshComponent> ArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<USceneComponent> BucketPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<UStaticMeshComponent> BucketMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hierarchy")
	TObjectPtr<USceneComponent> BucketTip;

	// Orugas y ruedas motrices (F2.05)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> TrackLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> TrackRightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> SprocketLeftFront;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> SprocketLeftRear;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> SprocketRightFront;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Tracks")
	TObjectPtr<UStaticMeshComponent> SprocketRightRear;

	// Cilindros hidraulicos (F2.04)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> BoomCylinderLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> BoomCylinderRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> ArmCylinderLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> ArmCylinderRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> BucketCylinderLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Hydraulics")
	TObjectPtr<UStaticMeshComponent> BucketCylinderRight;

	// Camaras especializadas (F2.06)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<UCameraComponent> CabinCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<USpringArmComponent> BucketBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<UCameraComponent> BucketCamera;

	// Audio placeholder (F2.07)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Engine")
	TObjectPtr<UAudioComponent> EngineAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Engine")
	TObjectPtr<UAudioComponent> HydraulicAudio;

	// Acciones Enhanced Input de excavacion
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ArmAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TurretAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BucketAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleModeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CycleCameraAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> EngineAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DigCycleAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator")
	ECWExcavatorControlMode ControlMode = ECWExcavatorControlMode::Dig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator")
	int32 ActiveCameraIndex = 0; // 0 = Exterior, 1 = Cabina, 2 = Cuchara

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Engine")
	bool bEngineRunning = false;

	// Angulos actuales (grados)
	float CurrentTurretYaw = 0.f;
	float CurrentBoomPitch = 15.f;
	float CurrentArmPitch = 10.f;
	float CurrentBucketPitch = 0.f;

	// Entradas de excavacion recibidas cada frame
	float TargetTurretInput = 0.f;
	float TargetBoomInput = 0.f;
	float TargetArmInput = 0.f;
	float TargetBucketInput = 0.f;

	// Orugas
	float LeftTrackSpeed = 0.f;
	float RightTrackSpeed = 0.f;
	float SprocketAngleDeg = 0.f;

	// Carga de la cuchara (m3)
	float BucketLoadCubicMeters = 0.f;

	// Estado de audio
	bool bHydraulicAudioActive = false;
};
