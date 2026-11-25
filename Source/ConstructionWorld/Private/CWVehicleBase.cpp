#include "CWVehicleBase.h"

#include "CWInputConfig.h"
#include "CWPlayerCharacter.h"
#include "CWLog.h"
#include "CWNotificationSubsystem.h"
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
	// La caja termina 20 cm por encima de la base de la carroceria (GroundContactDepthCm):
	// esa holgura deja pasar baches y pendientes; la altura la fija UpdateGroundFollow.
	CollisionBox->SetBoxExtent(FVector(200.f, 100.f, 80.f));
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
		UpdateGroundFollow(DeltaSeconds);
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

	UpdateGroundFollow(DeltaSeconds);
	DriveInput = FVector2D::ZeroVector;
}

void ACWVehicleBase::BeginPlay()
{
	Super::BeginPlay();
	SnapToGround();
}

int32 ACWVehicleBase::SampleGroundPlane(float& OutCenterZ, float& OutSlopeX, float& OutSlopeY) const
{
	OutCenterZ = 0.f;
	OutSlopeX = 0.f;
	OutSlopeY = 0.f;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<FVector2D, TInlineAllocator<8>> Points;
	if (GroundContactPoints.Num() > 0)
	{
		Points.Append(GroundContactPoints);
	}
	else
	{
		const FVector Ext = CollisionBox ? CollisionBox->GetUnscaledBoxExtent() : FVector(200.f, 100.f, 80.f);
		Points.Add(FVector2D( Ext.X, -Ext.Y));
		Points.Add(FVector2D( Ext.X,  Ext.Y));
		Points.Add(FVector2D(-Ext.X, -Ext.Y));
		Points.Add(FVector2D(-Ext.X,  Ext.Y));
	}

	const FVector Location = GetActorLocation();
	const FQuat YawQuat = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();

	// Solo geometria del mundo (estatica y dinamica): ni peatones ni el propio conductor.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(TEXT("CWGroundFollow"), false, this);
	if (Driver)
	{
		Params.AddIgnoredActor(Driver);
	}

	const float TanMaxTilt = FMath::Tan(FMath::DegreesToRadians(MaxGroundTiltDegrees));

	// Ajuste por minimos cuadrados de z = a + b*x + c*y en ejes locales (solo yaw).
	double N = 0, Sx = 0, Sy = 0, Sz = 0, Sxx = 0, Syy = 0, Sxy = 0, Sxz = 0, Syz = 0;
	for (const FVector2D& P : Points)
	{
		const FVector XY = Location + YawQuat.RotateVector(FVector(P.X, P.Y, 0.f));
		// Empezar por encima de lo que el chasis podria tener delante en la pendiente maxima.
		const float Reach = MaxStepHeightCm + (FMath::Abs(P.X) + FMath::Abs(P.Y)) * TanMaxTilt;
		const FVector Start(XY.X, XY.Y, Location.Z - GroundContactDepthCm + Reach);
		const FVector End(XY.X, XY.Y, Location.Z - GroundContactDepthCm - 2000.f);

		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params) || Hit.bStartPenetrating)
		{
			continue;
		}

		const double Z = Hit.ImpactPoint.Z;
		N += 1; Sx += P.X; Sy += P.Y; Sz += Z;
		Sxx += P.X * P.X; Syy += P.Y * P.Y; Sxy += P.X * P.Y;
		Sxz += P.X * Z; Syz += P.Y * Z;
	}

	if (N < 1)
	{
		return 0;
	}

	OutCenterZ = static_cast<float>(Sz / N);
	if (N < 3)
	{
		return static_cast<int32>(N);
	}

	// Sistema normal 3x3 resuelto por Cramer.
	const double M[3][3] = { { N, Sx, Sy }, { Sx, Sxx, Sxy }, { Sy, Sxy, Syy } };
	const double V[3] = { Sz, Sxz, Syz };
	auto Det3 = [](const double A[3][3])
	{
		return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
			- A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
			+ A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
	};
	const double D = Det3(M);
	if (FMath::Abs(D) < 1e-3)
	{
		return static_cast<int32>(N);
	}

	double Mi[3][3];
	for (int32 Col = 0; Col < 3; ++Col)
	{
		FMemory::Memcpy(Mi, M, sizeof(M));
		for (int32 Row = 0; Row < 3; ++Row)
		{
			Mi[Row][Col] = V[Row];
		}
		const double Value = Det3(Mi) / D;
		if (Col == 0)
		{
			OutCenterZ = static_cast<float>(Value);
		}
		else if (Col == 1)
		{
			OutSlopeX = static_cast<float>(FMath::Clamp<double>(Value, -TanMaxTilt, TanMaxTilt));
		}
		else
		{
			OutSlopeY = static_cast<float>(FMath::Clamp<double>(Value, -TanMaxTilt, TanMaxTilt));
		}
	}

	return static_cast<int32>(N);
}

void ACWVehicleBase::UpdateGroundFollow(float DeltaSeconds)
{
	if (!bFollowGround || DeltaSeconds <= 0.f)
	{
		return;
	}

	float CenterZ = 0.f;
	float SlopeX = 0.f;
	float SlopeY = 0.f;
	const int32 Hits = SampleGroundPlane(CenterZ, SlopeX, SlopeY);

	const FVector Location = GetActorLocation();
	const FRotator Rotation = GetActorRotation();

	if (Hits == 0)
	{
		// Sin suelo en 20 m: mantener la posicion en vez de caer al vacio.
		bOnGround = false;
		VerticalSpeedCmS = 0.f;
		if (!bWarnedNoGround)
		{
			UE_LOG(LogCWVehicle, Warning, TEXT("%s: sin suelo bajo el vehiculo; se mantiene la altura."), *GetName());
			bWarnedNoGround = true;
		}
		return;
	}
	bWarnedNoGround = false;

	// El origen queda a GroundContactDepthCm del plano, medido sobre su normal.
	const float NormalScale = FMath::Sqrt(1.f + SlopeX * SlopeX + SlopeY * SlopeY);
	const float TargetZ = CenterZ + GroundContactDepthCm * NormalScale;
	const float Gap = Location.Z - TargetZ;

	float NewZ = Location.Z;
	if (Gap <= MaxStepHeightCm && VerticalSpeedCmS > -50.f)
	{
		// Pegado al suelo: sube y baja suavemente siguiendo baches y pendientes.
		NewZ = FMath::FInterpTo(Location.Z, TargetZ, DeltaSeconds, 18.f);
		VerticalSpeedCmS = 0.f;
		bOnGround = true;
	}
	else
	{
		// Hueco grande (borde de una zanja): caida libre hasta apoyar.
		VerticalSpeedCmS = FMath::Max(VerticalSpeedCmS - 980.f * DeltaSeconds, -3000.f);
		NewZ = Location.Z + VerticalSpeedCmS * DeltaSeconds;
		if (NewZ <= TargetZ)
		{
			NewZ = TargetZ;
			VerticalSpeedCmS = 0.f;
			bOnGround = true;
		}
		else
		{
			bOnGround = false;
		}
	}

	// Cabeceo positivo = morro arriba; alabeo positivo baja el lado derecho (+Y).
	FRotator TargetRotation = Rotation;
	if (Hits >= 3)
	{
		TargetRotation.Pitch = FMath::RadiansToDegrees(FMath::Atan(SlopeX));
		TargetRotation.Roll = -FMath::RadiansToDegrees(FMath::Atan(SlopeY));
	}
	FRotator NewRotation = FMath::RInterpTo(Rotation, TargetRotation, DeltaSeconds, GroundAlignSpeed);
	NewRotation.Yaw = Rotation.Yaw;

	const FQuat YawQuat = FRotator(0.f, Rotation.Yaw, 0.f).Quaternion();
	GroundPlaneNormal = YawQuat.RotateVector(FVector(-SlopeX, -SlopeY, 1.f).GetSafeNormal());

	SetActorLocationAndRotation(FVector(Location.X, Location.Y, NewZ), NewRotation, false, nullptr, ETeleportType::None);
}

void ACWVehicleBase::SnapToGround()
{
	if (!bFollowGround)
	{
		return;
	}

	float CenterZ = 0.f;
	float SlopeX = 0.f;
	float SlopeY = 0.f;
	const int32 Hits = SampleGroundPlane(CenterZ, SlopeX, SlopeY);
	if (Hits == 0)
	{
		return;
	}

	FRotator Rotation = GetActorRotation();
	if (Hits >= 3)
	{
		Rotation.Pitch = FMath::RadiansToDegrees(FMath::Atan(SlopeX));
		Rotation.Roll = -FMath::RadiansToDegrees(FMath::Atan(SlopeY));
	}
	const float NormalScale = FMath::Sqrt(1.f + SlopeX * SlopeX + SlopeY * SlopeY);
	FVector Location = GetActorLocation();
	Location.Z = CenterZ + GroundContactDepthCm * NormalScale;

	VerticalSpeedCmS = 0.f;
	bOnGround = true;
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
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
	UCWNotificationSubsystem::Notify(this, TEXT("No hay sitio libre para bajar: aparta la maquina de los obstaculos."), ECWNoticeSeverity::Warning);
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
