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
 * Fase 2: entrar/salir, camara de vehiculo y pose del conductor.
 * Fase 3: motor, conduccion, hidraulica y herramientas.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWVehicleBase : public APawn, public ICWInteractable
{
	GENERATED_BODY()

public:
	ACWVehicleBase();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "CW|Vehicle")
	bool TryEnter(ACWPlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "CW|Vehicle")
	void ExitVehicle();

	UFUNCTION(BlueprintPure, Category = "CW|Vehicle")
	ACWPlayerCharacter* GetDriver() const { return Driver; }

	// ICWInteractable
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	FName DriverSeatSocket = TEXT("DriverSeat");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Vehicle")
	FVector ExitOffset = FVector(-350.f, 0.f, 50.f);

protected:
	void ExitPressed(const FInputActionValue& Value);
	void VehicleLook(const FInputActionValue& Value);
	void EnsureInputObjects();
	void ApplyVehicleMappingContext();

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

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CW|Vehicle")
	TObjectPtr<ACWPlayerCharacter> Driver;
};
