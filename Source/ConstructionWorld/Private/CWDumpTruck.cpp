#include "CWDumpTruck.h"

#include "CWDumpZone.h"
#include "CWLog.h"
#include "CWTerrainSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ACWDumpTruck::ACWDumpTruck()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox->SetBoxExtent(FVector(330.f, 140.f, 90.f));

	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	BodyMesh->SetRelativeScale3D(FVector(6.4f, 2.6f, 1.5f));

	MaxDriveSpeed = 1000.f; // ~36 km/h descargado
	TurnRate = 60.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Caja basculante (F2.09/F2.11): pivote en la parte trasera.
	BedPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BedPivot"));
	BedPivot->SetupAttachment(CollisionBox);
	BedPivot->SetRelativeLocation(FVector(-180.f, 0.f, 95.f));

	BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
	BedMesh->SetupAttachment(BedPivot);
	BedMesh->SetRelativeLocation(FVector(200.f, 0.f, 45.f));
	BedMesh->SetRelativeScale3D(FVector(4.2f, 2.2f, 1.0f));
	BedMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BedMesh->SetCollisionResponseToAllChannels(ECR_Block);
	if (CubeMesh.Succeeded())
	{
		BedMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Cuatro ruedas (F2.09/F2.10). Eje a lo largo de Y (roll 90).
	auto CreateWheel = [this](const TCHAR* Name, const FVector& Location)
	{
		UStaticMeshComponent* Wheel = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Wheel->SetupAttachment(CollisionBox);
		Wheel->SetRelativeLocation(Location);
		Wheel->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		Wheel->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.7f));
		Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (CylinderMesh.Succeeded())
		{
			Wheel->SetStaticMesh(CylinderMesh.Object);
		}
		return Wheel;
	};

	WheelFrontLeft = CreateWheel(TEXT("WheelFrontLeft"), FVector(220.f, -125.f, -30.f));
	WheelFrontRight = CreateWheel(TEXT("WheelFrontRight"), FVector(220.f, 125.f, -30.f));
	WheelRearLeft = CreateWheel(TEXT("WheelRearLeft"), FVector(-220.f, -125.f, -30.f));
	WheelRearRight = CreateWheel(TEXT("WheelRearRight"), FVector(-220.f, 125.f, -30.f));
}

void ACWDumpTruck::BeginPlay()
{
	Super::BeginPlay();

	UpdateBed(0.f);
	UpdateWheels(0.f);

	UE_LOG(LogCWVehicle, Log, TEXT("CWDumpTruck listo (capacidad %.1f m3)."), CapacityCubicMeters);
}

void ACWDumpTruck::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	ACWVehicleBase::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsValid(BedAction))
	{
		BedAction = NewObject<UInputAction>(this, TEXT("IA_DumpBed"));
		BedAction->ValueType = EInputActionValueType::Boolean;

		DumpAction = NewObject<UInputAction>(this, TEXT("IA_DumpLoad"));
		DumpAction->ValueType = EInputActionValueType::Boolean;

		VehicleMappingContext->MapKey(BedAction, EKeys::B);
		VehicleMappingContext->MapKey(DumpAction, EKeys::T);
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(BedAction, ETriggerEvent::Started, this, &ACWDumpTruck::BedInput);
		EnhancedInput->BindAction(DumpAction, ETriggerEvent::Started, this, &ACWDumpTruck::DumpInput);
	}
}

void ACWDumpTruck::Tick(float DeltaSeconds)
{
	// Modelo de ruedas por traza (F2.10). No usamos el movimiento cinematico
	// del vehiculo base: aqui hay inercia, agarre, pendiente y peso.
	if (!Driver)
	{
		CurrentSpeedCmS = FMath::FInterpTo(CurrentSpeedCmS, 0.f, DeltaSeconds, 2.f);
		SteerAngle = FMath::FInterpTo(SteerAngle, 0.f, DeltaSeconds, 5.f);
		DriveInput = FVector2D::ZeroVector;
		UpdateBed(DeltaSeconds);
		UpdateWheels(DeltaSeconds);
		return;
	}

	FVector GroundNormal = FVector::UpVector;
	const float Traction = GetTractionAndNormal(GroundNormal);

	const float Fill = GetFillRatio();
	const float LoadFactor = 1.f - LoadSpeedPenalty * Fill;
	const float Throttle = DriveInput.X;
	const float Steer = DriveInput.Y;

	if (Throttle > KINDA_SMALL_NUMBER)
	{
		CurrentSpeedCmS += AccelerationCmS2 * LoadFactor * Traction * DeltaSeconds;
	}
	else if (Throttle < -KINDA_SMALL_NUMBER)
	{
		CurrentSpeedCmS -= AccelerationCmS2 * LoadFactor * Traction * DeltaSeconds;
	}
	else
	{
		CurrentSpeedCmS = FMath::FInterpTo(CurrentSpeedCmS, 0.f, DeltaSeconds, 1.1f);
	}

	// Componente de la gravedad sobre el plano de apoyo (sube = frena, baja = acelera).
	const float SlopeDot = FVector::DotProduct(GetActorForwardVector(), GroundNormal);
	CurrentSpeedCmS += SlopeDot * 980.f * 0.35f * DeltaSeconds;

	if (GroundedWheelCount == 0)
	{
		CurrentSpeedCmS = FMath::FInterpTo(CurrentSpeedCmS, 0.f, DeltaSeconds, 2.f);
	}

	const float MaxSpeed = MaxDriveSpeed * LoadFactor * FMath::Max(0.25f, Traction);
	CurrentSpeedCmS = FMath::Clamp(CurrentSpeedCmS, -MaxSpeed * 0.5f, MaxSpeed);

	// Direccion: solo gira si se mueve (no hay giro estacionario en ruedas).
	if (!FMath::IsNearlyZero(Steer) && FMath::Abs(CurrentSpeedCmS) > 1.f)
	{
		const float SpeedRatio = CurrentSpeedCmS / FMath::Max(1.f, MaxDriveSpeed);
		AddActorLocalRotation(FRotator(0.f, Steer * TurnRate * SpeedRatio * DeltaSeconds, 0.f));
	}

	SteerAngle = FMath::FInterpTo(SteerAngle, Steer * MaxSteerAngle, DeltaSeconds, 6.f);

	if (!FMath::IsNearlyZero(CurrentSpeedCmS))
	{
		FHitResult Hit;
		AddActorWorldOffset(GetActorForwardVector() * CurrentSpeedCmS * DeltaSeconds, true, &Hit);
		if (Hit.bBlockingHit)
		{
			CurrentSpeedCmS *= 0.1f;
		}
	}

	// Seguridad F2.11: no se permite circular con la caja levantada.
	if (CurrentBedAngle > 5.f && FMath::Abs(CurrentSpeedCmS) > MaxDumpSpeedCmS)
	{
		if (TargetBedAngle > 0.f)
		{
			UE_LOG(LogCWVehicle, Warning,
				TEXT("CWDumpTruck: caja levantada en movimiento; se ordena el retorno."));
			TargetBedAngle = 0.f;
		}
	}

	UpdateBed(DeltaSeconds);
	UpdateWheels(DeltaSeconds);

	DriveInput = FVector2D::ZeroVector;
}

float ACWDumpTruck::GetTractionAndNormal(FVector& OutGroundNormal)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		OutGroundNormal = FVector::UpVector;
		GroundedWheelCount = 0;
		return 0.f;
	}

	const UStaticMeshComponent* Wheels[] = {
		WheelFrontLeft, WheelFrontRight, WheelRearLeft, WheelRearRight
	};

	FVector NormalSum = FVector::ZeroVector;
	GroundedWheelCount = 0;

	FCollisionQueryParams Params(TEXT("CWWheelGround"), false, this);

	for (const UStaticMeshComponent* Wheel : Wheels)
	{
		if (!Wheel)
		{
			continue;
		}

		const FVector WheelLocation = Wheel->GetComponentLocation();
		const FVector Start = WheelLocation + FVector(0.f, 0.f, 120.f);
		const FVector End = WheelLocation - FVector(0.f, 0.f, WheelRadiusCm + 120.f);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			++GroundedWheelCount;
			NormalSum += Hit.ImpactNormal;
		}
	}

	if (GroundedWheelCount > 0)
	{
		OutGroundNormal = NormalSum.GetSafeNormal();
		if (OutGroundNormal.IsNearlyZero())
		{
			OutGroundNormal = FVector::UpVector;
		}
	}
	else
	{
		OutGroundNormal = FVector::UpVector;
	}

	return static_cast<float>(GroundedWheelCount) / 4.f;
}

void ACWDumpTruck::UpdateBed(float DeltaSeconds)
{
	if (!BedPivot)
	{
		return;
	}

	if (CurrentBedAngle < TargetBedAngle)
	{
		CurrentBedAngle = FMath::Min(TargetBedAngle, CurrentBedAngle + BedSpeed * DeltaSeconds);
	}
	else if (CurrentBedAngle > TargetBedAngle)
	{
		CurrentBedAngle = FMath::Max(TargetBedAngle, CurrentBedAngle - BedSpeed * DeltaSeconds);
	}

	BedPivot->SetRelativeRotation(FRotator(CurrentBedAngle, 0.f, 0.f));
}

void ACWDumpTruck::UpdateWheels(float DeltaSeconds)
{
	WheelSpinDeg = FMath::Fmod(
		WheelSpinDeg + (CurrentSpeedCmS / FMath::Max(1.f, WheelRadiusCm))
			* FMath::RadiansToDegrees(1.f) * DeltaSeconds,
		360.f);

	// Eje de la rueda a lo largo de Y: roll 90 + giro de rodadura.
	const FRotator RearRotation(0.f, 0.f, 90.f + WheelSpinDeg);
	const FRotator FrontRotation(0.f, SteerAngle, 90.f + WheelSpinDeg);

	if (WheelFrontLeft)  { WheelFrontLeft->SetRelativeRotation(FrontRotation); }
	if (WheelFrontRight) { WheelFrontRight->SetRelativeRotation(FrontRotation); }
	if (WheelRearLeft)   { WheelRearLeft->SetRelativeRotation(RearRotation); }
	if (WheelRearRight)  { WheelRearRight->SetRelativeRotation(RearRotation); }
}

float ACWDumpTruck::AddLoad(float CubicMeters)
{
	if (CubicMeters <= 0.f)
	{
		return 0.f;
	}

	const float FreeSpace = FMath::Max(0.f, CapacityCubicMeters - LoadCubicMeters);
	const float Accepted = FMath::Min(CubicMeters, FreeSpace);
	LoadCubicMeters += Accepted;

	const float Overflow = CubicMeters - Accepted;
	if (Overflow > KINDA_SMALL_NUMBER)
	{
		SpilledCubicMeters += Overflow;
		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWDumpTruck: %.2f m3 no caben en la caja y se derraman (total derramado %.2f m3)."),
			Overflow, SpilledCubicMeters);
	}

	if (Accepted > KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogCWVehicle, Log,
			TEXT("CWDumpTruck: cargados %.2f m3 (%.2f / %.1f m3, %.0f kg)."),
			Accepted, LoadCubicMeters, CapacityCubicMeters, GetLoadWeightKg());
	}

	return Accepted;
}

float ACWDumpTruck::RemoveLoad(float CubicMeters)
{
	if (CubicMeters <= 0.f)
	{
		return 0.f;
	}

	const float Removed = FMath::Min(CubicMeters, LoadCubicMeters);
	LoadCubicMeters -= Removed;
	return Removed;
}

void ACWDumpTruck::ToggleBed()
{
	SetBedTarget(TargetBedAngle > 0.f ? 0.f : MaxBedAngle);
}

void ACWDumpTruck::SetBedTarget(float AngleDegrees)
{
	const float Clamped = FMath::Clamp(AngleDegrees, 0.f, MaxBedAngle);

	if (Clamped > CurrentBedAngle + KINDA_SMALL_NUMBER
		&& FMath::Abs(CurrentSpeedCmS) > MaxDumpSpeedCmS)
	{
		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWDumpTruck: deten el camion antes de levantar la caja (%.0f cm/s)."),
			CurrentSpeedCmS);
		return;
	}

	TargetBedAngle = Clamped;
}

void ACWDumpTruck::SetBedAngleForTest(float AngleDegrees)
{
	CurrentBedAngle = FMath::Clamp(AngleDegrees, 0.f, MaxBedAngle);
	TargetBedAngle = CurrentBedAngle;
	UpdateBed(0.f);
}

bool ACWDumpTruck::TryDump()
{
	if (CurrentBedAngle < MinDumpAngle)
	{
		UE_LOG(LogCWVehicle, Warning,
			TEXT("CWDumpTruck: la caja debe estar levantada (%.0f/%.0f grados)."),
			CurrentBedAngle, MinDumpAngle);
		return false;
	}

	if (LoadCubicMeters <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogCWVehicle, Warning, TEXT("CWDumpTruck: no hay material que descargar."));
		return false;
	}

	if (FMath::Abs(CurrentSpeedCmS) > MaxDumpSpeedCmS)
	{
		UE_LOG(LogCWVehicle, Warning, TEXT("CWDumpTruck: el camion debe estar parado para descargar."));
		return false;
	}

	ACWDumpZone* Zone = FindDumpZone();
	const float Amount = LoadCubicMeters;

	if (Zone)
	{
		if (Zone->DeliverLoad(Amount, this))
		{
			LoadCubicMeters = 0.f;
			UE_LOG(LogCWVehicle, Log, TEXT("CWDumpTruck: descargados %.2f m3 en zona."), Amount);
			return true;
		}

		return false;
	}

	// H2: sin zona, descargar directamente en el terreno (conserva el volumen).
	if (UWorld* World = GetWorld())
	{
		if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
		{
			const float Deposited = Terrain->DepositAt(
				GetBedCenterLocation(), DumpRadiusCm, Amount, LoadSoilType);

			if (Deposited > KINDA_SMALL_NUMBER)
			{
				LoadCubicMeters = FMath::Max(0.f, LoadCubicMeters - Deposited);
				UE_LOG(LogCWVehicle, Log,
					TEXT("CWDumpTruck: descargados %.2f m3 de %s en el terreno (resto %.2f m3)."),
					Deposited, *CWSoil::GetSoilProperties(LoadSoilType).DisplayName,
					LoadCubicMeters);
				return true;
			}
		}
	}

	UE_LOG(LogCWVehicle, Warning,
		TEXT("CWDumpTruck: no hay zona de descarga ni terreno debajo del camion."));
	return false;
}

ACWDumpZone* ACWDumpTruck::FindDumpZone() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACWDumpZone> It(World); It; ++It)
	{
		ACWDumpZone* Zone = *It;
		if (Zone && Zone->IsActorInside(this))
		{
			return Zone;
		}
	}

	return nullptr;
}

bool ACWDumpTruck::IsInsideDumpZone() const
{
	return FindDumpZone() != nullptr;
}

FVector ACWDumpTruck::GetBedCenterLocation() const
{
	return IsValid(BedMesh) ? BedMesh->GetComponentLocation() : GetActorLocation();
}

void ACWDumpTruck::BedInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		ToggleBed();
	}
}

void ACWDumpTruck::DumpInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		TryDump();
	}
}
