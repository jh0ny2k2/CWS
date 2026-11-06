#include "CWVehicleBase.h"

#include "CWPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ACWVehicleBase::ACWVehicleBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetBoxExtent(FVector(200.f, 100.f, 100.f));
	CollisionBox->SetCollisionProfileName(TEXT("BlockAll"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(CollisionBox);
	BodyMesh->SetRelativeScale3D(FVector(4.f, 2.f, 2.f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CollisionBox);
	CameraBoom->TargetArmLength = 700.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 150.f);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ACWVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(ExitAction, ETriggerEvent::Started, this, &ACWVehicleBase::ExitPressed);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACWVehicleBase::VehicleLook);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CWVehicleBase: se esperaba un UEnhancedInputComponent."));
	}

	ApplyVehicleMappingContext();
}

void ACWVehicleBase::EnsureInputObjects()
{
	if (IsValid(VehicleMappingContext))
	{
		return;
	}

	ExitAction = NewObject<UInputAction>(this, TEXT("IA_ExitVehicle"));
	ExitAction->ValueType = EInputActionValueType::Boolean;

	LookAction = NewObject<UInputAction>(this, TEXT("IA_VehicleLook"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	VehicleMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Vehicle"));
	VehicleMappingContext->MapKey(ExitAction, EKeys::F);
	VehicleMappingContext->MapKey(LookAction, EKeys::Mouse2D);
}

void ACWVehicleBase::ApplyVehicleMappingContext()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(VehicleMappingContext, 1);
	}
}

bool ACWVehicleBase::TryEnter(ACWPlayerCharacter* Player)
{
	if (!Player || Driver)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC)
	{
		return false;
	}

	Driver = Player;

	Player->SetActorHiddenInGame(true);
	Player->SetActorEnableCollision(false);
	if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}
	Player->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale, DriverSeatSocket);

	PC->Possess(this);
	return true;
}

void ACWVehicleBase::ExitVehicle()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(VehicleMappingContext);
	}

	PC->UnPossess();

	if (Driver)
	{
		Driver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Driver->SetActorEnableCollision(true);
		Driver->SetActorHiddenInGame(false);

		if (UCharacterMovementComponent* Movement = Driver->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}

		const FVector ExitLocation = GetActorLocation() + GetActorRotation().RotateVector(ExitOffset);
		Driver->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);

		ACWPlayerCharacter* OldDriver = Driver;
		Driver = nullptr;
		PC->Possess(OldDriver);
	}
}

void ACWVehicleBase::ExitPressed(const FInputActionValue& Value)
{
	ExitVehicle();
}

void ACWVehicleBase::VehicleLook(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void ACWVehicleBase::Interact_Implementation(AActor* Interactor)
{
	if (ACWPlayerCharacter* Player = Cast<ACWPlayerCharacter>(Interactor))
	{
		TryEnter(Player);
	}
}

FText ACWVehicleBase::GetInteractionPrompt_Implementation() const
{
	if (Driver)
	{
		return NSLOCTEXT("CW", "VehicleOccupied", "Vehiculo ocupado");
	}

	return NSLOCTEXT("CW", "EnterVehicle", "Entrar en el vehiculo [E]");
}
