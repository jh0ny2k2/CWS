#include "CWExcavator.h"

#include "CWDumpTruck.h"
#include "CWLog.h"
#include "CWTerrainField.h"
#include "CWTerrainSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACWExcavator::ACWExcavator()
{
	PrimaryActorTick.bCanEverTick = true;

	// Chasis/orugas: caja algo mas ancha y baja que el vehiculo base.
	CollisionBox->SetBoxExtent(FVector(260.f, 150.f, 70.f));

	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	BodyMesh->SetRelativeScale3D(FVector(5.2f, 3.f, 1.4f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Orugas izquierda/derecha (F2.05): dos cajas largas bajo el chasis.
	TrackLeftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrackLeftMesh"));
	TrackLeftMesh->SetupAttachment(CollisionBox);
	TrackLeftMesh->SetRelativeLocation(FVector(0.f, -110.f, -45.f));
	TrackLeftMesh->SetRelativeScale3D(FVector(5.2f, 0.8f, 0.5f));
	TrackLeftMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrackRightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrackRightMesh"));
	TrackRightMesh->SetupAttachment(CollisionBox);
	TrackRightMesh->SetRelativeLocation(FVector(0.f, 110.f, -45.f));
	TrackRightMesh->SetRelativeScale3D(FVector(5.2f, 0.8f, 0.5f));
	TrackRightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (CubeMesh.Succeeded())
	{
		TrackLeftMesh->SetStaticMesh(CubeMesh.Object);
		TrackRightMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Ruedas motrices visibles: giran con la velocidad de cada oruga.
	auto CreateSprocket = [this](const TCHAR* Name, const FVector& Location)
	{
		UStaticMeshComponent* Sprocket = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Sprocket->SetupAttachment(CollisionBox);
		Sprocket->SetRelativeLocation(Location);
		Sprocket->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		Sprocket->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f));
		Sprocket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (CylinderMesh.Succeeded())
		{
			Sprocket->SetStaticMesh(CylinderMesh.Object);
		}
		return Sprocket;
	};

	SprocketLeftFront = CreateSprocket(TEXT("SprocketLeftFront"), FVector(220.f, -110.f, -45.f));
	SprocketLeftRear = CreateSprocket(TEXT("SprocketLeftRear"), FVector(-220.f, -110.f, -45.f));
	SprocketRightFront = CreateSprocket(TEXT("SprocketRightFront"), FVector(220.f, 110.f, -45.f));
	SprocketRightRear = CreateSprocket(TEXT("SprocketRightRear"), FVector(-220.f, 110.f, -45.f));

	// Torreta: gira sobre Z y arrastra cabina y brazo.
	TurretMount = CreateDefaultSubobject<USceneComponent>(TEXT("TurretMount"));
	TurretMount->SetupAttachment(CollisionBox);
	TurretMount->SetRelativeLocation(FVector(-40.f, 0.f, 70.f));

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(TurretMount);
	TurretMesh->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	TurretMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 1.6f));
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	// Cilindros hidraulicos (F2.04): se recolocan cada frame entre sus anclajes.
	auto CreateCylinder = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Cylinder->SetupAttachment(CollisionBox);
		Cylinder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Cylinder->SetRelativeScale3D(FVector(0.16f, 0.16f, 1.f));
		if (CylinderMesh.Succeeded())
		{
			Cylinder->SetStaticMesh(CylinderMesh.Object);
		}
		return Cylinder;
	};

	BoomCylinderLeft = CreateCylinder(TEXT("BoomCylinderLeft"));
	BoomCylinderRight = CreateCylinder(TEXT("BoomCylinderRight"));
	ArmCylinderLeft = CreateCylinder(TEXT("ArmCylinderLeft"));
	ArmCylinderRight = CreateCylinder(TEXT("ArmCylinderRight"));
	BucketCylinderLeft = CreateCylinder(TEXT("BucketCylinderLeft"));
	BucketCylinderRight = CreateCylinder(TEXT("BucketCylinderRight"));

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

	// Audio placeholder (F2.07): sin asset asignado no suena, pero el estado
	// de motor/hidraulica se registra en log y se expone al HUD.
	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(CollisionBox);
	EngineAudio->bAutoActivate = false;

	HydraulicAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HydraulicAudio"));
	HydraulicAudio->SetupAttachment(TurretMount);
	HydraulicAudio->bAutoActivate = false;

	// Estado inicial coherente con los angulos declarados en la cabecera.
	ApplyJointRotations();
}

void ACWExcavator::BeginPlay()
{
	Super::BeginPlay();

	// La maquina arranca con el motor en marcha y deposito lleno.
	FuelLiters = FMath::Clamp(FuelLiters, 0.f, MaxFuelLiters);
	StartEngine();
	UpdateHydraulics();
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
	EnhancedInput->BindAction(EngineAction, ETriggerEvent::Started, this, &ACWExcavator::EngineInput);
	EnhancedInput->BindAction(DigCycleAction, ETriggerEvent::Started, this, &ACWExcavator::DigCycleInput);
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

	EngineAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorEngine"));
	EngineAction->ValueType = EInputActionValueType::Boolean;

	DigCycleAction = NewObject<UInputAction>(this, TEXT("IA_ExcavatorDigCycle"));
	DigCycleAction->ValueType = EInputActionValueType::Boolean;

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
	VehicleMappingContext->MapKey(EngineAction, EKeys::G);
	VehicleMappingContext->MapKey(DigCycleAction, EKeys::T);
}

void ACWExcavator::Tick(float DeltaSeconds)
{
	UpdateEngine(DeltaSeconds);

	if (!Driver || !bEngineRunning)
	{
		TargetTurretInput = 0.f;
		TargetBoomInput = 0.f;
		TargetArmInput = 0.f;
		TargetBucketInput = 0.f;
		DriveInput = FVector2D::ZeroVector;
		LeftTrackSpeed = 0.f;
		RightTrackSpeed = 0.f;
		ApplyJointRotations();
		UpdateHydraulics();
		return;
	}

	if (ControlMode == ECWExcavatorControlMode::Drive)
	{
		// Traslado con orugas: diferencial izquierda/derecha (F2.05).
		UpdateTracks(DeltaSeconds);
		DriveInput = FVector2D::ZeroVector;
	}
	else
	{
		// Excavacion: la base no debe moverse aunque se pulsen W/S/A/D.
		DriveInput = FVector2D::ZeroVector;
		LeftTrackSpeed = 0.f;
		RightTrackSpeed = 0.f;

		CurrentTurretYaw += TargetTurretInput * TurretSpeed * DeltaSeconds;
		if (!bContinuousTurret)
		{
			CurrentTurretYaw = FMath::Clamp(CurrentTurretYaw, MinTurretYaw, MaxTurretYaw);
		}
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

	ApplyJointRotations();
	UpdateHydraulics();

	// Audio hidraulico placeholder: suena solo si hay asset y movimiento de brazo.
	const bool bHydraulicMoving = !FMath::IsNearlyZero(TargetBoomInput)
		|| !FMath::IsNearlyZero(TargetArmInput)
		|| !FMath::IsNearlyZero(TargetBucketInput)
		|| !FMath::IsNearlyZero(TargetTurretInput);

	if (HydraulicSound && HydraulicAudio)
	{
		if (bHydraulicMoving && !bHydraulicAudioActive)
		{
			HydraulicAudio->SetSound(HydraulicSound);
			HydraulicAudio->Play();
			bHydraulicAudioActive = true;
		}
		else if (!bHydraulicMoving && bHydraulicAudioActive)
		{
			HydraulicAudio->Stop();
			bHydraulicAudioActive = false;
		}
	}

	TargetTurretInput = 0.f;
	TargetBoomInput = 0.f;
	TargetArmInput = 0.f;
	TargetBucketInput = 0.f;
}

void ACWExcavator::UpdateEngine(float DeltaSeconds)
{
	if (!bEngineRunning)
	{
		return;
	}

	// Consumo: ralenti ~35 %, plena carga cuando hay demanda de movimiento.
	const float Demand = FMath::Clamp(
		FMath::Abs(DriveInput.X) + FMath::Abs(DriveInput.Y) * 0.5f
		+ (FMath::Abs(TargetBoomInput) + FMath::Abs(TargetArmInput) + FMath::Abs(TargetBucketInput)) * 0.3f,
		0.f, 1.f);
	const float LoadFactor = 0.35f + 0.65f * Demand;

	FuelLiters -= FuelConsumptionLitersPerHour * LoadFactor * (DeltaSeconds / 3600.f);

	if (FuelLiters <= 0.f)
	{
		FuelLiters = 0.f;
		UE_LOG(LogCWVehicle, Warning, TEXT("CWExcavator: sin combustible; el motor se detiene."));
		StopEngine();
	}
}

void ACWExcavator::UpdateTracks(float DeltaSeconds)
{
	const float Forward = DriveInput.X;
	const float Turn = DriveInput.Y;

	// Diferencial: A/D giran en sentido contrario -> giro estacionario.
	float Left = FMath::Clamp(Forward - Turn, -1.f, 1.f);
	float Right = FMath::Clamp(Forward + Turn, -1.f, 1.f);

	// "Agarre": cada oruga solo transmite fuerza si tiene suelo debajo.
	const UWorld* World = GetWorld();
	auto HasGroundUnder = [World, this](const FVector& Location) -> bool
	{
		if (!World)
		{
			return false;
		}
		FHitResult Hit;
		FCollisionQueryParams Params(TEXT("CWTrackGround"), false, this);
		const FVector Start = Location + FVector(0.f, 0.f, 80.f);
		const FVector End = Location - FVector(0.f, 0.f, 160.f);
		return World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
	};

	const bool bLeftGrounded = HasGroundUnder(TrackLeftMesh->GetComponentLocation());
	const bool bRightGrounded = HasGroundUnder(TrackRightMesh->GetComponentLocation());

	const float Grip = (bLeftGrounded ? 0.5f : 0.f) + (bRightGrounded ? 0.5f : 0.f);
	if (Grip <= 0.f)
	{
		LeftTrackSpeed = 0.f;
		RightTrackSpeed = 0.f;
		return;
	}

	Left *= Grip;
	Right *= Grip;

	LeftTrackSpeed = Left * MaxDriveSpeed;
	RightTrackSpeed = Right * MaxDriveSpeed;

	const float LinearSpeed = (LeftTrackSpeed + RightTrackSpeed) * 0.5f;

	// Giro diferencial: velocidad angular = (v_dcha - v_izda) / ancho de via.
	const float YawDegPerSecond =
		(RightTrackSpeed - LeftTrackSpeed) / FMath::Max(1.f, TrackHalfWidthCm * 2.f)
		* FMath::RadiansToDegrees(1.f);

	if (!FMath::IsNearlyZero(YawDegPerSecond))
	{
		AddActorLocalRotation(FRotator(0.f, YawDegPerSecond * DeltaSeconds, 0.f));
	}

	if (!FMath::IsNearlyZero(LinearSpeed))
	{
		FHitResult Hit;
		AddActorWorldOffset(GetActorForwardVector() * LinearSpeed * DeltaSeconds, true, &Hit);
		if (Hit.bBlockingHit)
		{
			LeftTrackSpeed = 0.f;
			RightTrackSpeed = 0.f;
		}
	}

	// Animacion de las ruedas motrices (visual).
	SprocketAngleDeg = FMath::Fmod(SprocketAngleDeg + LinearSpeed * DeltaSeconds * 0.45f, 360.f);
	const FRotator WheelRot(0.f, 0.f, 90.f + SprocketAngleDeg);

	for (UStaticMeshComponent* Sprocket : {
		SprocketLeftFront.Get(), SprocketLeftRear.Get(),
		SprocketRightFront.Get(), SprocketRightRear.Get() })
	{
		if (Sprocket)
		{
			Sprocket->SetRelativeRotation(WheelRot);
		}
	}
}

void ACWExcavator::ApplyJointRotations()
{
	if (!TurretMount || !BoomPivot || !ArmPivot || !BucketPivot)
	{
		return;
	}

	TurretMount->SetRelativeRotation(FRotator(0.f, CurrentTurretYaw, 0.f));
	BoomPivot->SetRelativeRotation(FRotator(CurrentBoomPitch, 0.f, 0.f));
	ArmPivot->SetRelativeRotation(FRotator(CurrentArmPitch, 0.f, 0.f));
	BucketPivot->SetRelativeRotation(FRotator(CurrentBucketPitch, 0.f, 0.f));
}

void ACWExcavator::UpdateHydraulics()
{
	if (!TurretMount || !BoomPivot || !ArmPivot || !BucketPivot)
	{
		return;
	}

	// Anclajes reales de cada cilindro, en coordenadas locales del componente
	// que los soporta. Se transforman a mundo para colocar el cilindro.
	const FTransform TurretXf = TurretMount->GetComponentTransform();
	const FTransform BoomXf = BoomPivot->GetComponentTransform();
	const FTransform ArmXf = ArmPivot->GetComponentTransform();
	const FTransform BucketXf = BucketPivot->GetComponentTransform();

	for (const float Side : { -1.f, 1.f })
	{
		const FVector BoomA = TurretXf.TransformPosition(FVector(40.f, Side * 55.f, 40.f));
		const FVector BoomB = BoomXf.TransformPosition(FVector(160.f, Side * 55.f, -30.f));
		PlaceCylinder(Side < 0.f ? BoomCylinderLeft.Get() : BoomCylinderRight.Get(), BoomA, BoomB, 0.17f);

		const FVector ArmA = BoomXf.TransformPosition(FVector(250.f, Side * 45.f, -15.f));
		const FVector ArmB = ArmXf.TransformPosition(FVector(80.f, Side * 45.f, 0.f));
		PlaceCylinder(Side < 0.f ? ArmCylinderLeft.Get() : ArmCylinderRight.Get(), ArmA, ArmB, 0.15f);

		const FVector BucketA = ArmXf.TransformPosition(FVector(190.f, Side * 40.f, 0.f));
		const FVector BucketB = BucketXf.TransformPosition(FVector(50.f, Side * 40.f, 0.f));
		PlaceCylinder(Side < 0.f ? BucketCylinderLeft.Get() : BucketCylinderRight.Get(), BucketA, BucketB, 0.13f);
	}
}

void ACWExcavator::PlaceCylinder(UStaticMeshComponent* Cylinder, const FVector& WorldA,
	const FVector& WorldB, float RadiusScale) const
{
	if (!Cylinder)
	{
		return;
	}

	const FVector Delta = WorldB - WorldA;
	const float Length = Delta.Size();
	if (Length < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector MidPoint = (WorldA + WorldB) * 0.5f;
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Delta).Rotator();

	Cylinder->SetWorldLocationAndRotation(MidPoint, Rotation);
	Cylinder->SetWorldScale3D(FVector(RadiusScale, RadiusScale, Length / 100.f));
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
	LeftTrackSpeed = 0.f;
	RightTrackSpeed = 0.f;

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

void ACWExcavator::StartEngine()
{
	if (bEngineRunning)
	{
		return;
	}

	if (FuelLiters <= 0.f)
	{
		UE_LOG(LogCWVehicle, Warning, TEXT("CWExcavator: no se puede arrancar sin combustible."));
		return;
	}

	bEngineRunning = true;

	if (EngineSound && EngineAudio)
	{
		EngineAudio->SetSound(EngineSound);
		EngineAudio->Play();
	}

	UE_LOG(LogCWVehicle, Log, TEXT("CWExcavator: motor en marcha (%.1f L de combustible)."), FuelLiters);
}

void ACWExcavator::StopEngine()
{
	if (!bEngineRunning)
	{
		return;
	}

	bEngineRunning = false;
	TargetTurretInput = 0.f;
	TargetBoomInput = 0.f;
	TargetArmInput = 0.f;
	TargetBucketInput = 0.f;
	LeftTrackSpeed = 0.f;
	RightTrackSpeed = 0.f;

	if (EngineAudio)
	{
		EngineAudio->Stop();
	}
	if (HydraulicAudio)
	{
		HydraulicAudio->Stop();
		bHydraulicAudioActive = false;
	}

	UE_LOG(LogCWVehicle, Log, TEXT("CWExcavator: motor detenido."));
}

void ACWExcavator::ToggleEngine()
{
	if (bEngineRunning)
	{
		StopEngine();
	}
	else
	{
		StartEngine();
	}
}

float ACWExcavator::Refuel(float Liters)
{
	if (Liters <= 0.f)
	{
		return 0.f;
	}

	const float Before = FuelLiters;
	FuelLiters = FMath::Clamp(FuelLiters + Liters, 0.f, MaxFuelLiters);
	return FuelLiters - Before;
}

bool ACWExcavator::DigOrLoadTruck()
{
	if (!bEngineRunning)
	{
		UE_LOG(LogCWVehicle, Warning, TEXT("CWExcavator: el motor debe estar en marcha para excavar."));
		return false;
	}

	if (BucketLoadCubicMeters > KINDA_SMALL_NUMBER)
	{
		if (ACWDumpTruck* Truck = FindNearbyTruck())
		{
			Truck->SetLoadSoilType(BucketSoilType);
			const float Moved = Truck->AddLoad(BucketLoadCubicMeters);
			if (Moved > KINDA_SMALL_NUMBER)
			{
				BucketLoadCubicMeters = FMath::Max(0.f, BucketLoadCubicMeters - Moved);
				UE_LOG(LogCWVehicle, Log,
					TEXT("CWExcavator: descargados %.2f m3 en el camion (cuchara: %.2f m3)."),
					Moved, BucketLoadCubicMeters);
				return true;
			}
		}

		// H2: si no hay camion, verter la cuchara en el terreno (conserva el volumen).
		if (UWorld* World = GetWorld())
		{
			if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
			{
				const float Deposited = Terrain->DepositAt(
					GetBucketTipLocation(), DepositRadiusCm, BucketLoadCubicMeters, BucketSoilType);

				if (Deposited > KINDA_SMALL_NUMBER)
				{
					BucketLoadCubicMeters = FMath::Max(0.f, BucketLoadCubicMeters - Deposited);
					UE_LOG(LogCWVehicle, Log,
						TEXT("CWExcavator: vertidos %.2f m3 de %s en el terreno (cuchara: %.2f m3)."),
						Deposited, *GetBucketSoilName(), BucketLoadCubicMeters);
					return true;
				}
			}
		}

		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWExcavator: no hay camion ni terreno al alcance para vaciar la cuchara."));
		return false;
	}

	// H2: excavar de verdad en el terreno deformable.
	if (UWorld* World = GetWorld())
	{
		if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
		{
			const FVector Tip = GetBucketTipLocation();
			if (ACWTerrainField* Field = Terrain->FindTerrainAt(Tip))
			{
				const float SurfaceZ = Field->GetHeightAtLocation(Tip);
				if (Tip.Z - SurfaceZ <= DigReachCm)
				{
					ECWSoilType SoilType = ECWSoilType::Earth;
					const float Removed = Terrain->DigAt(
						Tip, DigRadiusCm, MaxDigDepthCm, BucketCapacityCubicMeters, SoilType);

					if (Removed > KINDA_SMALL_NUMBER)
					{
						BucketLoadCubicMeters = FMath::Min(Removed, BucketCapacityCubicMeters);
						BucketSoilType = SoilType;
						UE_LOG(LogCWVehicle, Log,
							TEXT("CWExcavator: cuchara llena con %.2f m3 de %s del terreno."),
							BucketLoadCubicMeters, *GetBucketSoilName());
						return true;
					}
				}
			}
		}
	}

	if (!IsBucketTipNearGround())
	{
		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWExcavator: la cuchara no esta cerca del suelo (%.0f cm)."), DigReachCm);
		return false;
	}

	BucketLoadCubicMeters = BucketCapacityCubicMeters;
	BucketSoilType = ECWSoilType::Earth;
	UE_LOG(LogCWVehicle, Log, TEXT("CWExcavator: cuchara llena (%.2f m3)."), BucketLoadCubicMeters);
	return true;
}

FString ACWExcavator::GetBucketSoilName() const
{
	return CWSoil::GetSoilProperties(BucketSoilType).DisplayName;
}

void ACWExcavator::SetBucketLoadForTest(float CubicMeters)
{
	BucketLoadCubicMeters = FMath::Clamp(CubicMeters, 0.f, BucketCapacityCubicMeters);
	BucketSoilType = ECWSoilType::Earth;
}

ACWDumpTruck* ACWExcavator::FindNearbyTruck() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Tip = GetBucketTipLocation();
	ACWDumpTruck* Best = nullptr;
	float BestDistance = TruckLoadReachCm;

	for (TActorIterator<ACWDumpTruck> It(World); It; ++It)
	{
		ACWDumpTruck* Truck = *It;
		if (!Truck || !Truck->CanAcceptLoad())
		{
			continue;
		}

		const float Distance = FVector::Dist(Tip, Truck->GetBedCenterLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Best = Truck;
		}
	}

	return Best;
}

bool ACWExcavator::IsBucketTipNearGround() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("CWDigGround"), false, this);
	for (TActorIterator<ACWVehicleBase> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}

	const FVector Start = GetBucketTipLocation() + FVector(0.f, 0.f, 20.f);
	const FVector End = Start - FVector(0.f, 0.f, DigReachCm);
	return World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}

float ACWExcavator::GetStabilityMarginCm(float SlopeDegrees, float SlopeDirectionDegrees) const
{
	struct FMassPoint
	{
		FVector LocalPosition = FVector::ZeroVector;
		float MassKg = 0.f;
	};

	TArray<FMassPoint> Points;
	const FTransform ActorXf = GetActorTransform();

	auto AddPoint = [&Points, &ActorXf](const USceneComponent* Component, float MassKg)
	{
		if (Component && MassKg > 0.f)
		{
			FMassPoint Point;
			Point.LocalPosition = ActorXf.InverseTransformPositionNoScale(Component->GetComponentLocation());
			Point.MassKg = MassKg;
			Points.Add(Point);
		}
	};

	AddPoint(CollisionBox, ChassisMassKg);
	AddPoint(TurretMount, TurretMassKg);
	AddPoint(BoomMesh, BoomMassKg);
	AddPoint(ArmMesh, ArmMassKg);
	AddPoint(BucketMesh, BucketMassKg);

	const float LoadMassKg = BucketLoadCubicMeters * CWSoil::GetDensityKgPerCubicMeter(BucketSoilType);
	AddPoint(BucketTip, LoadMassKg);

	float TotalMass = 0.f;
	FVector WeightedPosition = FVector::ZeroVector;
	for (const FMassPoint& Point : Points)
	{
		TotalMass += Point.MassKg;
		WeightedPosition += Point.LocalPosition * Point.MassKg;
	}

	if (TotalMass <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const FVector CenterOfMass = WeightedPosition / TotalMass;

	// Direccion de la gravedad en el mundo para una pendiente dada.
	const float SlopeRad = FMath::DegreesToRadians(SlopeDegrees);
	const float DirectionRad = FMath::DegreesToRadians(SlopeDirectionDegrees);
	const FVector WorldGravity(
		FMath::Sin(SlopeRad) * FMath::Cos(DirectionRad),
		FMath::Sin(SlopeRad) * FMath::Sin(DirectionRad),
		-FMath::Cos(SlopeRad));

	const FVector LocalGravity = ActorXf.InverseTransformVectorNoScale(WorldGravity);
	const float GravityZ = FMath::Min(-0.1f, LocalGravity.Z);

	// Proyeccion del centro de masas sobre el plano de apoyo (z local = 0).
	const FVector GroundProjection(
		CenterOfMass.X - CenterOfMass.Z * LocalGravity.X / GravityZ,
		CenterOfMass.Y - CenterOfMass.Z * LocalGravity.Y / GravityZ,
		0.f);

	const float MarginX = TrackHalfLengthCm - FMath::Abs(GroundProjection.X);
	const float MarginY = TrackHalfWidthCm - FMath::Abs(GroundProjection.Y);

	return FMath::Min(MarginX, MarginY);
}

bool ACWExcavator::IsStableOnSlope(float SlopeDegrees, float SlopeDirectionDegrees) const
{
	return GetStabilityMarginCm(SlopeDegrees, SlopeDirectionDegrees) >= SafetyMarginCm;
}

void ACWExcavator::SetTestPose(float TurretYaw, float BoomPitch, float ArmPitch, float BucketPitch)
{
	CurrentTurretYaw = bContinuousTurret
		? TurretYaw
		: FMath::Clamp(TurretYaw, MinTurretYaw, MaxTurretYaw);
	CurrentBoomPitch = FMath::Clamp(BoomPitch, MinBoomPitch, MaxBoomPitch);
	CurrentArmPitch = FMath::Clamp(ArmPitch, MinArmPitch, MaxArmPitch);
	CurrentBucketPitch = FMath::Clamp(BucketPitch, MinBucketPitch, MaxBucketPitch);

	ApplyJointRotations();
	UpdateHydraulics();
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

void ACWExcavator::EngineInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		ToggleEngine();
	}
}

void ACWExcavator::DigCycleInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		DigOrLoadTruck();
	}
}
