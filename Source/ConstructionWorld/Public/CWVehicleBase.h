#pragma once

#include "CoreMinimal.h"
#include "CWInteractable.h"
#include "GameFramework/Pawn.h"
#include "CWVehicleBase.generated.h"

class ACWPlayerCharacter;
class UBoxComponent;
class UCameraComponent;
class UCWVehicleFXComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * Base de maquinaria/vehiculo (Fase 2-3).
 *
 * Fase 2: entrar/salir, camara de vehiculo, conduccion basica WASD y pose del conductor.
 * Fase 3: motor, conduccion avanzada, hidraulica y herramientas.
 *
 * BUG-006 corregido: ExitVehicle hace sweep para buscar espacio libre antes de teleportar.
 * BUG-007 mitigado: AttachToComponent usa la malla del cuerpo; DriverSeat puede no existir
 *   en el placeholder (Unreal usa el origen de la malla si el socket no existe, que es
 *   seguro). Se registra el aviso en log.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWVehicleBase : public APawn, public ICWInteractable
{
	GENERATED_BODY()

public:
	ACWVehicleBase();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "CW|Vehicle")
	bool TryEnter(ACWPlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "CW|Vehicle")
	void ExitVehicle();

	UFUNCTION(BlueprintPure, Category = "CW|Vehicle")
	ACWPlayerCharacter* GetDriver() const { return Driver; }

	UFUNCTION(BlueprintPure, Category = "CW|Vehicle|FX")
	UCWVehicleFXComponent* GetVisualFX() const { return VisualFX; }

	// ICWInteractable
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Socket de la malla donde se adjunta el conductor. Puede no existir en el placeholder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	FName DriverSeatSocket = TEXT("DriverSeat");

	/** Desplazamiento local respecto al vehiculo al salir. Se prueba con sweep; si no hay
	 *  espacio se buscan candidatos alternativos a izquierda, derecha y atras. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	FVector ExitOffset = FVector(-350.f, 0.f, 50.f);

	/** Velocidad de avance maxima del vehiculo (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Drive",
		meta = (ClampMin = "0.0", ClampMax = "3000.0"))
	float MaxDriveSpeed = 900.f;

	/** Velocidad de giro maxima del vehiculo (grados/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Drive",
		meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float TurnRate = 80.f;

	/** Sigue el suelo: altura, cabeceo y alabeo a partir de trazas en los puntos de apoyo,
	 *  con caida libre si debajo hay un hueco mayor que MaxStepHeightCm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground")
	bool bFollowGround = true;

	/** Distancia del origen del actor a la base de ruedas/orugas (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground", meta = (ClampMin = "0.0"))
	float GroundContactDepthCm = 100.f;

	/** Puntos de apoyo locales (X, Y) en cm. Vacio = esquinas de la caja de colision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground")
	TArray<FVector2D> GroundContactPoints;

	/** Desnivel que se sigue pegado al suelo; por encima se cae con gravedad (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground", meta = (ClampMin = "0.0"))
	float MaxStepHeightCm = 60.f;

	/** Rapidez con la que el chasis se alinea con la pendiente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground", meta = (ClampMin = "0.1"))
	float GroundAlignSpeed = 6.f;

	/** Inclinacion maxima que adopta el chasis (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Vehicle|Ground", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float MaxGroundTiltDegrees = 35.f;

	UFUNCTION(BlueprintPure, Category = "CW|Vehicle|Ground")
	bool IsOnGround() const { return bOnGround; }

	/** Normal del plano de apoyo calculado en el ultimo frame. */
	UFUNCTION(BlueprintPure, Category = "CW|Vehicle|Ground")
	FVector GetGroundPlaneNormal() const { return GroundPlaneNormal; }

	/** Fuerza un paso de seguimiento del suelo (tests y colocacion inicial). */
	UFUNCTION(BlueprintCallable, Category = "CW|Vehicle|Ground")
	void SnapToGround();

protected:
	/** Ajusta altura y orientacion al suelo. Llamar cada frame despues del avance. */
	void UpdateGroundFollow(float DeltaSeconds);

	virtual void BeginPlay() override;

	void ExitPressed(const FInputActionValue& Value);
	void VehicleLook(const FInputActionValue& Value);
	void VehicleLookGamepad(const FInputActionValue& Value);
	void VehicleMoveForward(const FInputActionValue& Value);
	void VehicleMoveRight(const FInputActionValue& Value);
	void EnsureInputObjects();
	void ApplyVehicleMappingContext();

	/** Busca un punto de salida libre: capsula sin solape, suelo debajo (max. MaxExitDrop) y
	 *  linea de vision desde la cabina para no salir al otro lado de un muro. Si ninguno vale,
	 *  devuelve false y OutLocation queda con el candidato por defecto. */
	bool FindSafeExitLocation(FVector& OutLocation) const;

	/** Caida maxima aceptada bajo un punto de salida (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	float MaxExitDrop = 400.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Polvo de excavacion/carga y huellas de orugas (H4 / F5.04). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle|FX")
	TObjectPtr<UCWVehicleFXComponent> VisualFX;

	/** Punto de anclaje fisico del conductor dentro de la cabina (F5.06). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle|Seat")
	TObjectPtr<USceneComponent> DriverSeatPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> VehicleMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ExitAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookGamepadAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DriveForwardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DriveRightAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<ACWPlayerCharacter> Driver;

	/** Entrada de movimiento acumulada cada frame. */
	FVector2D DriveInput = FVector2D::ZeroVector;

private:
	/** Calcula el plano de apoyo. Devuelve el numero de puntos con suelo. */
	int32 SampleGroundPlane(float& OutCenterZ, float& OutSlopeX, float& OutSlopeY) const;

	float VerticalSpeedCmS = 0.f;
	bool bOnGround = false;
	bool bWarnedNoGround = false;
	FVector GroundPlaneNormal = FVector::UpVector;
};
