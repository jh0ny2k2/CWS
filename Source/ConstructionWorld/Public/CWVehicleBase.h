#pragma once

#include "CoreMinimal.h"
#include "CWInteractable.h"
#include "GameFramework/Pawn.h"
#include "CWVehicleBase.generated.h"

class ACWPlayerCharacter;
class UBoxComponent;
class UCameraComponent;
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

	// ICWInteractable
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

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

protected:
	void ExitPressed(const FInputActionValue& Value);
	void VehicleLook(const FInputActionValue& Value);
	void VehicleMoveForward(const FInputActionValue& Value);
	void VehicleMoveRight(const FInputActionValue& Value);
	void EnsureInputObjects();
	void ApplyVehicleMappingContext();

	/** Busca un punto de salida libre con sweep de capsula. Devuelve el primero encontrado
	 *  o la posicion por defecto si ninguno esta libre. */
	FVector FindSafeExitLocation() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

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
	TObjectPtr<UInputAction> DriveForwardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DriveRightAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<ACWPlayerCharacter> Driver;

	/** Entrada de movimiento acumulada cada frame. */
	FVector2D DriveInput = FVector2D::ZeroVector;
};
