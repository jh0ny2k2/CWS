#pragma once

#include "CoreMinimal.h"
#include "CWVehicleBase.h"
#include "CWExcavator.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class USceneComponent;
class UInputAction;
class UInputMappingContext;
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
 *     -> TurretMesh (Cabina/Torreta giratoria)
 *       -> CabinCamera (Vista interior)
 *       -> BoomMesh (Pluma principal)
 *         -> ArmMesh (Balancin/Brazo secundario)
 *           -> BucketMesh (Cuchara)
 *             -> BucketTip (Punto de excavacion)
 *             -> BucketCamera (Camara de precision)
 *
 * Tareas H1-B cubiertas:
 *   - F2.01: Geometria jerarquica con pivotes articulados.
 *   - F2.02: Rotacion de torreta continua/limite.
 *   - F2.03: Pluma, brazo y cuchara con limites angulares e interpolacion suave.
 *   - F2.06: Multiples camaras (exterior, cabina y enfoque a cuchara con tecla C).
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWExcavator : public ACWVehicleBase
{
	GENERATED_BODY()

public:
	ACWExcavator();

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

	// Limites de rotacion (grados)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Excavator|Limits")
	float TurretSpeed = 45.f; // deg/s

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

protected:
	virtual void EnsureInputObjects();

	// Handlers de entrada
	void DigBoomInput(const FInputActionValue& Value);
	void DigArmInput(const FInputActionValue& Value);
	void DigTurretInput(const FInputActionValue& Value);
	void DigBucketInput(const FInputActionValue& Value);
	void ToggleModeInput(const FInputActionValue& Value);
	void CycleCameraInput(const FInputActionValue& Value);

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

	// Camaras especializadas (F2.06)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<UCameraComponent> CabinCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<USpringArmComponent> BucketBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator|Cameras")
	TObjectPtr<UCameraComponent> BucketCamera;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator")
	ECWExcavatorControlMode ControlMode = ECWExcavatorControlMode::Dig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Excavator")
	int32 ActiveCameraIndex = 0; // 0 = Exterior, 1 = Cabina, 2 = Cuchara

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
};
