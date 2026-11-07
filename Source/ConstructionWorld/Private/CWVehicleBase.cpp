#include "CWVehicleBase.h"

#include "CWPlayerCharacter.h"
#include "CWLog.h"
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
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

ACWVehicleBase::ACWVehicleBase()
{
	// Tick necesario para mover el vehiculo cada frame
	PrimaryActorTick.bCanEverTick = true;

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
		EnhancedInput->BindAction(ExitAction,         ETriggerEvent::Started,   this, &ACWVehicleBase::ExitPressed);
		EnhancedInput->BindAction(LookAction,         ETriggerEvent::Triggered, this, &ACWVehicleBase::VehicleLook);
		EnhancedInput->BindAction(DriveForwardAction, ETriggerEvent::Triggered, this, &ACWVehicleBase::VehicleMoveForward);
		EnhancedInput->BindAction(DriveRightAction,   ETriggerEvent::Triggered, this, &ACWVehicleBase::VehicleMoveRight);
	}
	else
	{
		UE_LOG(LogCWVehicle, Warning, TEXT("CWVehicleBase: se esperaba un UEnhancedInputComponent."));
	}

	ApplyVehicleMappingContext();
}

void ACWVehicleBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Driver)
	{
		return;
	}

	// Rotacion sobre Z (giro izquierda/derecha con A/D)
	if (!FMath::IsNearlyZero(DriveInput.Y))
	{
		const FRotator DeltaRot(0.f, DriveInput.Y * TurnRate * DeltaSeconds, 0.f);
		AddActorLocalRotation(DeltaRot);
	}

	// Desplazamiento adelante/atras con W/S
	if (!FMath::IsNearlyZero(DriveInput.X))
	{
		const FVector Delta = GetActorForwardVector() * DriveInput.X * MaxDriveSpeed * DeltaSeconds;
		AddActorWorldOffset(Delta, true);
	}

	DriveInput = FVector2D::ZeroVector;
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

	DriveForwardAction = NewObject<UInputAction>(this, TEXT("IA_DriveForward"));
	DriveForwardAction->ValueType = EInputActionValueType::Axis1D;

	DriveRightAction = NewObject<UInputAction>(this, TEXT("IA_DriveRight"));
	DriveRightAction->ValueType = EInputActionValueType::Axis1D;

	VehicleMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Vehicle"));

	VehicleMappingContext->MapKey(ExitAction, EKeys::F);
	VehicleMappingContext->MapKey(LookAction, EKeys::Mouse2D);

	// WASD para conducir
	VehicleMappingContext->MapKey(DriveForwardAction, EKeys::W);
	FEnhancedActionKeyMapping& BackMap = VehicleMappingContext->MapKey(DriveForwardAction, EKeys::S);
	BackMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	VehicleMappingContext->MapKey(DriveRightAction, EKeys::D);
	FEnhancedActionKeyMapping& LeftMap = VehicleMappingContext->MapKey(DriveRightAction, EKeys::A);
	LeftMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));
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

FVector ACWVehicleBase::FindSafeExitLocation() const
{
	const UWorld* World = GetWorld();
	if (!Driver || !World)
	{
		return GetActorLocation() + GetActorRotation().RotateVector(ExitOffset);
	}

	// Capsula del personaje para el sweep
	const float HalfHeight = 96.f;
	const float Radius     = 42.f;
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	FCollisionQueryParams Params(TEXT("ExitSweep"), false, this);
	Params.AddIgnoredActor(Driver);

	// Candidatos en orden de preferencia: atras, derecha, izquierda, delante
	const FVector LocalOffsets[] = {
		ExitOffset,
		FVector(0.f,  FMath::Abs(ExitOffset.X), ExitOffset.Z),
		FVector(0.f, -FMath::Abs(ExitOffset.X), ExitOffset.Z),
		FVector( FMath::Abs(ExitOffset.X), 0.f, ExitOffset.Z)
	};

	for (const FVector& LocalOff : LocalOffsets)
	{
		const FVector WorldCandidate = GetActorLocation() + GetActorRotation().RotateVector(LocalOff);
		FHitResult Hit;
		const bool bBlocked = World->SweepSingleByChannel(
			Hit, WorldCandidate, WorldCandidate,
			FQuat::Identity, ECC_Pawn, CapsuleShape, Params);

		if (!bBlocked)
		{
			return WorldCandidate;
		}
	}

	UE_LOG(LogCWVehicle, Warning,
		TEXT("CWVehicleBase::FindSafeExitLocation - no se encontro un punto libre; "
		     "usando offset por defecto."));
	return GetActorLocation() + GetActorRotation().RotateVector(ExitOffset);
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

	// BUG-007 mitigado: adjuntamos al BodyMesh. Si DriverSeat no existe en el placeholder
	// Unreal usa el origen del mesh (comportamiento seguro). Se avisa en log.
	if (!BodyMesh->DoesSocketExist(DriverSeatSocket))
	{
		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWVehicleBase: El socket '%s' no existe en BodyMesh. "
			     "Se usara el origen del mesh hasta que se modele la cabina."),
			*DriverSeatSocket.ToString());
	}
	Player->AttachToComponent(
		BodyMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		DriverSeatSocket);

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

		// BUG-006 corregido: sweep para encontrar un punto libre antes de teleportar
		const FVector SafeLocation = FindSafeExitLocation();
		Driver->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);

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

void ACWVehicleBase::VehicleMoveForward(const FInputActionValue& Value)
{
	DriveInput.X = Value.Get<float>();
}

void ACWVehicleBase::VehicleMoveRight(const FInputActionValue& Value)
{
	DriveInput.Y = Value.Get<float>();
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
