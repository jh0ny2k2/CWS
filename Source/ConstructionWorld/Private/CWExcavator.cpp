#include "CWExcavator.h"

#include "CWLog.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

ACWExcavator::ACWExcavator()
{
	PrimaryActorTick.bCanEverTick = true;

	// Chasis/orugas: caja algo mas ancha y baja que el vehiculo base.
	CollisionBox->SetBoxExtent(FVector(260.f, 150.f, 70.f));

	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	BodyMesh->SetRelativeScale3D(FVector(5.2f, 3.f, 1.4f));

	// Torreta: gira sobre Z y arrastra cabina y brazo.
	TurretMount = CreateDefaultSubobject<USceneComponent>(TEXT("TurretMount"));
	TurretMount->SetupAttachment(CollisionBox);
	TurretMount->SetRelativeLocation(FVector(-40.f, 0.f, 70.f));

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(TurretMount);
	TurretMesh->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	TurretMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 1.6f));
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		TurretMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Camara de cabina (F2.06): fija a la torreta.
	CabinCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CabinCamera"));
	CabinCamera->SetupAttachment(TurretMount);
	CabinCamera->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	CabinCamera->SetRelativeRotation(FRotator(-5.f, 0.f, 0.f));
	CabinCamera->bUsePawnControlRotation = true;
	CabinCamera->bAutoActivate = false;

	// Pluma principal: pivota sobre la torreta.
	BoomPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BoomPivot"));
	BoomPivot->SetupAttachment(TurretMount);
	BoomPivot->SetRelativeLocation(FVector(60.f, 0.f, 120.f));

	BoomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoomMesh"));
	BoomMesh->SetupAttachment(BoomPivot);
	BoomMesh->SetRelativeLocation(FVector(170.f, 0.f, 0.f));
	BoomMesh->SetRelativeScale3D(FVector(3.4f, 0.5f, 0.5f));
	BoomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded())
	{
		BoomMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Balancin/brazo secundario: pivota en la punta de la pluma.
	ArmPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ArmPivot"));
	ArmPivot->SetupAttachment(BoomPivot);
	ArmPivot->SetRelativeLocation(FVector(340.f, 0.f, 0.f));

	ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmMesh"));
	ArmMesh->SetupAttachment(ArmPivot);
	ArmMesh->SetRelativeLocation(FVector(130.f, 0.f, 0.f));
	ArmMesh->SetRelativeScale3D(FVector(2.6f, 0.4f, 0.4f));
	ArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded())
	{
		ArmMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Cuchara: pivota en la punta del balancin.
	BucketPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BucketPivot"));
	BucketPivot->SetupAttachment(ArmPivot);
	BucketPivot->SetRelativeLocation(FVector(260.f, 0.f, 0.f));

	BucketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BucketMesh"));
	BucketMesh->SetupAttachment(BucketPivot);
	BucketMesh->SetRelativeLocation(FVector(45.f, 0.f, -35.f));
	BucketMesh->SetRelativeScale3D(FVector(0.9f, 1.3f, 0.8f));
	BucketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded())
	{
		BucketMesh->SetStaticMesh(CubeMesh.Object);
	}

	BucketTip = CreateDefaultSubobject<USceneComponent>(TEXT("BucketTip"));
	BucketTip->SetupAttachment(BucketPivot);
	BucketTip->SetRelativeLocation(FVector(90.f, 0.f, -60.f));

	// Camara de precision enfocada a la cuchara (F2.06).
	BucketBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BucketBoom"));
	BucketBoom->SetupAttachment(BucketPivot);
	BucketBoom->TargetArmLength = 650.f;
	BucketBoom->SocketOffset = FVector(0.f, 0.f, 120.f);
	BucketBoom->bUsePawnControlRotation = true;
	BucketBoom->bDoCollisionTest = true;

	BucketCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BucketCamera"));
	BucketCamera->SetupAttachment(BucketBoom, USpringArmComponent::SocketName);
	BucketCamera->bUsePawnControlRotation = false;
	BucketCamera->bAutoActivate = false;

	// Estado inicial coherente con los angulos declarados en la cabecera.
	TurretMount->SetRelativeRotation(FRotator(0.f, CurrentTurretYaw, 0.f));
	BoomPivot->SetRelativeRotation(FRotator(CurrentBoomPitch, 0.f, 0.f));
	ArmPivot->SetRelativeRotation(FRotator(CurrentArmPitch, 0.f, 0.f));
	BucketPivot->SetRelativeRotation(FRotator(CurrentBucketPitch, 0.f, 0.f));
}

void ACWExcavator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Enlaza salir, mirar y conduccion (WASD) del vehiculo base.
	ACWVehicleBase::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogCWInput, Warning, TEXT("CWExcavator: se esperaba un UEnhancedInputComponent."));
		return;
	}

	EnhancedInput->BindAction(BoomAction, ETriggerEvent::Triggered, this, &ACWExcavator::DigBoomInput);
	EnhancedInput->BindAction(ArmAction, ETriggerEvent::Triggered, this, &ACWExcavator::DigArmInput);
	EnhancedInput->BindAction(TurretAction, ETriggerEvent::Triggered, this, &ACWExcavator::DigTurretInput);
	EnhancedInput->BindAction(BucketAction, ETriggerEvent::Triggered, this, &ACWExcavator::DigBucketInput);
	EnhancedInput->BindAction(ToggleModeAction, ETriggerEvent::Started, this, &ACWExcavator::ToggleModeInput);
	EnhancedInput->BindAction(CycleCameraAction, ETriggerEvent::Started, this, &ACWExcavator::CycleCameraInput);
}

void ACWExcavator::EnsureInputObjects()
{
	if (IsValid(BoomAction))
	{
		return;
	}

	// Crea el contexto y las acciones de conduccion del vehiculo base.
	ACWVehicleBase::EnsureInputObjects();

	BoomAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorBoom"));
	BoomAction->ValueType = EInputActionValueType::Axis1D;

	ArmAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorArm"));
	ArmAction->ValueType = EInputActionValueType::Axis1D;

	TurretAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorTurret"));
	TurretAction->ValueType = EInputActionValueType::Axis1D;

	BucketAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorBucket"));
	BucketAction->ValueType = EInputActionValueType::Axis1D;

	ToggleModeAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorToggleMode"));
	ToggleModeAction->ValueType = EInputActionValueType::Boolean;

	CycleCameraAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorCamera"));
	CycleCameraAction->ValueType = EInputActionValueType::Boolean;

	// Mapeos de excavacion. En modo traslado se reutilizan W/S/A/D para las orugas.
	VehicleMappingContext->MapKey(BoomAction, EKeys::W);
	FEnhancedActionKeyMapping& BoomDownMap = VehicleMappingContext->MapKey(BoomAction, EKeys::S);
	BoomDownMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	VehicleMappingContext->MapKey(ArmAction, EKeys::D);
	FEnhancedActionKeyMapping& ArmInMap = VehicleMappingContext->MapKey(ArmAction, EKeys::A);
	ArmInMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	VehicleMappingContext->MapKey(TurretAction, EKeys::E);
	FEnhancedActionKeyMapping& TurretLeftMap = VehicleMappingContext->MapKey(TurretAction, EKeys::Q);
	TurretLeftMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	VehicleMappingContext->MapKey(BucketAction, EKeys::Z);
	FEnhancedActionKeyMapping& BucketDownMap = VehicleMappingContext->MapKey(BucketAction, EKeys::X);
	BucketDownMap.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	VehicleMappingContext->MapKey(ToggleModeAction, EKeys::R);
	VehicleMappingContext->MapKey(CycleCameraAction, EKeys::C);
}

void ACWExcavator::Tick(float DeltaSeconds)
{
	if (!Driver)
	{
		TargetTurretInput = 0.f;
		TargetBoomInput = 0.f;
		TargetArmInput = 0.f;
		TargetBucketInput = 0.f;
		return;
	}

	if (ControlMode == ECWExcavatorControlMode::Drive)
	{
		// Traslado con orugas: comportamiento del vehiculo base.
		Super::Tick(DeltaSeconds);
	}
	else
	{
		// Excavacion: la base no debe moverse aunque se pulsen W/S/A/D.
		DriveInput = FVector2D::ZeroVector;

		CurrentTurretYaw += TargetTurretInput * TurretSpeed * DeltaSeconds;
		CurrentBoomPitch = FMath::Clamp(
			CurrentBoomPitch + TargetBoomInput * BoomSpeed * DeltaSeconds,
			MinBoomPitch, MaxBoomPitch);
		CurrentArmPitch = FMath::Clamp(
			CurrentArmPitch + TargetArmInput * ArmSpeed * DeltaSeconds,
			MinArmPitch, MaxArmPitch);
		CurrentBucketPitch = FMath::Clamp(
			CurrentBucketPitch + TargetBucketInput * BucketSpeed * DeltaSeconds,
			MinBucketPitch, MaxBucketPitch);
	}

	TurretMount->SetRelativeRotation(FRotator(0.f, CurrentTurretYaw, 0.f));
	BoomPivot->SetRelativeRotation(FRotator(CurrentBoomPitch, 0.f, 0.f));
	ArmPivot->SetRelativeRotation(FRotator(CurrentArmPitch, 0.f, 0.f));
	BucketPivot->SetRelativeRotation(FRotator(CurrentBucketPitch, 0.f, 0.f));

	TargetTurretInput = 0.f;
	TargetBoomInput = 0.f;
	TargetArmInput = 0.f;
	TargetBucketInput = 0.f;
}

void ACWExcavator::ToggleControlMode()
{
	ControlMode = (ControlMode == ECWExcavatorControlMode::Drive)
		? ECWExcavatorControlMode::Dig
		: ECWExcavatorControlMode::Drive;

	DriveInput = FVector2D::ZeroVector;
	TargetTurretInput = 0.f;
	TargetBoomInput = 0.f;
	TargetArmInput = 0.f;
	TargetBucketInput = 0.f;

	UE_LOG(LogCWVehicle, Log, TEXT("CWExcavator: modo de control -> %s"),
		ControlMode == ECWExcavatorControlMode::Drive ? TEXT("Conduccion") : TEXT("Excavacion"));
}

void ACWExcavator::CycleCamera()
{
	ActiveCameraIndex = (ActiveCameraIndex + 1) % 3;

	Camera->SetActive(ActiveCameraIndex == 0);
	CabinCamera->SetActive(ActiveCameraIndex == 1);
	BucketCamera->SetActive(ActiveCameraIndex == 2);

	UE_LOG(LogCWVehicle, Log, TEXT("CWExcavator: camara activa -> %s"),
		ActiveCameraIndex == 0 ? TEXT("Exterior")
			: (ActiveCameraIndex == 1 ? TEXT("Cabina") : TEXT("Cuchara")));
}

FVector ACWExcavator::GetBucketTipLocation() const
{
	return IsValid(BucketTip) ? BucketTip->GetComponentLocation() : GetActorLocation();
}

void ACWExcavator::DigBoomInput(const FInputActionValue& Value)
{
	TargetBoomInput = Value.Get<float>();
}

void ACWExcavator::DigArmInput(const FInputActionValue& Value)
{
	TargetArmInput = Value.Get<float>();
}

void ACWExcavator::DigTurretInput(const FInputActionValue& Value)
{
	TargetTurretInput = Value.Get<float>();
}

void ACWExcavator::DigBucketInput(const FInputActionValue& Value)
{
	TargetBucketInput = Value.Get<float>();
}

void ACWExcavator::ToggleModeInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		ToggleControlMode();
	}
}

void ACWExcavator::CycleCameraInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		CycleCamera();
	}
}
