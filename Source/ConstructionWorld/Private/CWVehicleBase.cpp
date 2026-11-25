#include "CWVehicleBase.h"

#include "CWInputConfig.h"
#include "CWPlayerCharacter.h"
#include "CWLog.h"
#include "CWVehicleFXComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/Engine.h"
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> YellowMat(
		TEXT("/Game/ConstructionWorld/Materials/MI_CW_ConstructionYellow.MI_CW_ConstructionYellow"));
	if (YellowMat.Succeeded())
	{
		BodyMesh->SetMaterial(0, YellowMat.Object);
	}

	// H4 (F5.04): polvo y huellas reutilizables por toda la maquinaria.
	VisualFX = CreateDefaultSubobject<UCWVehicleFXComponent>(TEXT("VisualFX"));
	VisualFX->SetupAttachment(CollisionBox);

	DriverSeatPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DriverSeatPoint"));
	DriverSeatPoint->SetupAttachment(CollisionBox);
	DriverSeatPoint->SetRelativeLocation(FVector(0.f, 0.f, 50.f));

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
		EnhancedInput->BindAction(LookGamepadAction,  ETriggerEvent::Triggered, this, &ACWVehicleBase::VehicleLookGamepad);
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

	LookGamepadAction = NewObject<UInputAction>(this, TEXT("IA_VehicleLookGamepad"));
	LookGamepadAction->ValueType = EInputActionValueType::Axis2D;

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

	// F1.07: mando. B sale, stick izquierdo conduce, stick derecho mira.
	VehicleMappingContext->MapKey(ExitAction, EKeys::Gamepad_FaceButton_Right);
	CWInput::MapKey(VehicleMappingContext, LookGamepadAction, EKeys::Gamepad_Right2D, this, false, true);
	CWInput::MapKey(VehicleMappingContext, DriveForwardAction, EKeys::Gamepad_LeftY, this, false, true);
	CWInput::MapKey(VehicleMappingContext, DriveRightAction, EKeys::Gamepad_LeftX, this, false, true);
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

bool ACWVehicleBase::FindSafeExitLocation(FVector& OutLocation) const
{
	const FVector DefaultLocation = GetActorLocation() + GetActorQuat().RotateVector(ExitOffset);
	OutLocation = DefaultLocation;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Capsula real del personaje (antes estaba fijada a mano a 42x96).
	float Radius = 42.f;
	float HalfHeight = 96.f;
	if (Driver && Driver->GetCapsuleComponent())
	{
		Driver->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
	}
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	FCollisionQueryParams Params(TEXT("ExitSweep"), false, this);
	Params.AddIgnoredActor(Driver);

	// Solo el giro en Z: en pendiente no queremos candidatos enterrados o en el aire.
	const FQuat YawQuat = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const float Side = FMath::Abs(ExitOffset.X);

	// Candidatos en orden de preferencia: atras, derecha, izquierda, delante y luego mas lejos.
	const FVector LocalOffsets[] = {
		ExitOffset,
		FVector(0.f,  Side, ExitOffset.Z),
		FVector(0.f, -Side, ExitOffset.Z),
		FVector(Side, 0.f, ExitOffset.Z),
		FVector(ExitOffset.X * 1.6f, 0.f, ExitOffset.Z),
		FVector(0.f,  Side * 1.6f, ExitOffset.Z),
		FVector(0.f, -Side * 1.6f, ExitOffset.Z)
	};

	const FVector CabinPoint = DriverSeatPoint ? DriverSeatPoint->GetComponentLocation() : GetActorLocation();

	for (const FVector& LocalOff : LocalOffsets)
	{
		const FVector Candidate = GetActorLocation() + YawQuat.RotateVector(LocalOff);

		// 1) Linea de vision desde la cabina: no atravesar muros ni otros vehiculos.
		FHitResult LosHit;
		if (World->LineTraceSingleByChannel(LosHit, CabinPoint, Candidate, ECC_Visibility, Params))
		{
			UE_LOG(LogCWVehicle, Verbose, TEXT("Salida %s: sin linea de vision (%s)"), *Candidate.ToCompactString(), *GetNameSafe(LosHit.GetActor()));
			continue;
		}

		// 2) Suelo: bajar la capsula desde el candidato hasta encontrar apoyo.
		FHitResult GroundHit;
		const FVector DropStart = Candidate + FVector(0.f, 0.f, HalfHeight);
		const FVector DropEnd = Candidate - FVector(0.f, 0.f, MaxExitDrop);
		if (!World->SweepSingleByChannel(GroundHit, DropStart, DropEnd, FQuat::Identity, ECC_Pawn, CapsuleShape, Params)
			|| GroundHit.bStartPenetrating
			|| GroundHit.ImpactNormal.Z < 0.6f)
		{
			UE_LOG(LogCWVehicle, Verbose, TEXT("Salida %s: sin suelo valido (hit=%d pen=%d actor=%s n=%s in=%s loc=%s)"), *Candidate.ToCompactString(), GroundHit.bBlockingHit, GroundHit.bStartPenetrating, *GetNameSafe(GroundHit.GetActor()), *GroundHit.Normal.ToCompactString(), *GroundHit.ImpactNormal.ToCompactString(), *GroundHit.Location.ToCompactString());
			continue;
		}

		// 3) Hueco libre en la posicion final (con 2 cm de margen sobre el suelo).
		const FVector Standing = GroundHit.Location + FVector(0.f, 0.f, 2.f);
		if (World->OverlapBlockingTestByChannel(Standing, FQuat::Identity, ECC_Pawn, CapsuleShape, Params))
		{
			UE_LOG(LogCWVehicle, Verbose, TEXT("Salida %s: hueco ocupado"), *Standing.ToCompactString());
			continue;
		}

		OutLocation = Standing;
		return true;
	}

	UE_LOG(LogCWVehicle, Warning,
		TEXT("CWVehicleBase::FindSafeExitLocation - %s sin punto de salida libre."), *GetName());
	return false;
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

	// Anclaje al puesto de conduccion (F5.06)
	if (DriverSeatPoint)
	{
		Player->AttachToComponent(
			DriverSeatPoint,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else if (BodyMesh->DoesSocketExist(DriverSeatSocket))
	{
		Player->AttachToComponent(
			BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			DriverSeatSocket);
	}
	else
	{
		Player->AttachToComponent(
			BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

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

	// Buscar el hueco antes de soltar nada: si no hay sitio, el jugador sigue al volante
	// en vez de aparecer dentro de un muro o caer por un talud (antes se forzaba la salida).
	FVector SafeLocation = FVector::ZeroVector;
	if (Driver && !FindSafeExitLocation(SafeLocation))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				static_cast<uint64>(GetUniqueID()), 2.5f, FColor::Orange,
				TEXT("No hay espacio para bajar del vehiculo aqui."));
		}
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(VehicleMappingContext);
	}

	PC->UnPossess();
	DriveInput = FVector2D::ZeroVector;

	if (Driver)
	{
		Driver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// De pie y mirando en la direccion del vehiculo (sin heredar inclinacion del chasis).
		const FRotator Upright(0.f, GetActorRotation().Yaw, 0.f);
		Driver->SetActorLocationAndRotation(SafeLocation, Upright, false, nullptr, ETeleportType::TeleportPhysics);
		Driver->SetActorEnableCollision(true);
		Driver->SetActorHiddenInGame(false);

		if (UCharacterMovementComponent* Movement = Driver->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}

		ACWPlayerCharacter* OldDriver = Driver;
		Driver = nullptr;
		PC->Possess(OldDriver);
		PC->SetControlRotation(Upright);
	}
}

void ACWVehicleBase::ExitPressed(const FInputActionValue& Value)
{
	ExitVehicle();
}

void ACWVehicleBase::VehicleLook(const FInputActionValue& Value)
{
	CWInput::ApplyLook(this, Value.Get<FVector2D>(), false);
}

void ACWVehicleBase::VehicleLookGamepad(const FInputActionValue& Value)
{
	CWInput::ApplyLook(this, Value.Get<FVector2D>(), true);
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

	return NSLOCTEXT("CW", "EnterVehicle", "Entrar en el vehiculo [E / X]");
}

bool ACWVehicleBase::CanInteract_Implementation(AActor* Interactor) const
{
	// Ocupado: se sigue enfocando para mostrar "Vehiculo ocupado", pero no por el propio conductor.
	return Interactor != Driver;
}
