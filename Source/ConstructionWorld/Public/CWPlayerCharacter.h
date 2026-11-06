#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CWPlayerCharacter.generated.h"

class UCameraComponent;
class UCWInteractionComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * Personaje jugador base (Fase 1-2).
 *
 * - Camara en tercera persona con spring arm.
 * - Movimiento relativo a la camara.
 * - Enhanced Input creado por codigo (sin assets binarios) para que el
 *   proyecto funcione nada mas compilar.
 * - Malla placeholder (cubo de Engine) hasta sustituirla por un personaje real.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACWPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void InteractInput(const FInputActionValue& Value);
	void ToggleCameraInput(const FInputActionValue& Value);

	/** Alterna entre camara de tercera persona y primera persona. */
	UFUNCTION(BlueprintCallable, Category = "CW|Camera")
	void ToggleCamera();

	/** Crea acciones y mapping context en tiempo de ejecucion (idempotente). */
	void EnsureInputObjects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Interaction")
	TObjectPtr<UCWInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Visual")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleCameraAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Camera")
	bool bFirstPerson = false;
};
