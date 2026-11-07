#include "CWTestRunner.h"

#include "CWLog.h"
#include "CWExcavator.h"
#include "CWDumpTruck.h"
#include "CWDumpZone.h"
#include "CWPlayerCharacter.h"
#include "CWStockpile.h"
#include "CWTerrainField.h"
#include "CWTerrainTypes.h"
#include "CWVehicleBase.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace
{
	int32 CountCharacters(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ACWPlayerCharacter> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	int32 CountVehicles(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ACWVehicleBase> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}
}

ACWTestRunner::ACWTestRunner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

void ACWTestRunner::StartEnterExitTest(int32 Cycles, float DelaySeconds, bool bInExitWhenDone)
{
	PendingCycles = FMath::Max(1, Cycles);
	bExitWhenDone = bInExitWhenDone;
	bFinished = false;

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST: programados %d ciclos de entrar/salir (retardo %.1f s)."),
		PendingCycles, DelaySeconds);

	GetWorldTimerManager().SetTimer(
		StartTimer, this, &ACWTestRunner::RunEnterExitTest,
		FMath::Max(0.1f, DelaySeconds), false);
}

bool ACWTestRunner::EnsureActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no hay mundo."));
		return false;
	}

	PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no hay PlayerController."));
		return false;
	}

	Character = Cast<ACWPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (!Character)
	{
		for (TActorIterator<ACWPlayerCharacter> It(World); It; ++It)
		{
			Character = *It;
			break;
		}
	}

	if (!Character)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se encontro ACWPlayerCharacter en el nivel."));
		return false;
	}

	if (Character->GetController() != PlayerController)
	{
		PlayerController->Possess(Character);
	}

	for (TActorIterator<ACWVehicleBase> It(World); It; ++It)
	{
		Vehicle = *It;
		break;
	}

	if (!Vehicle)
	{
		// Crear uno delante del personaje, apoyado en el suelo si hay traza.
		FVector SpawnLocation = Character->GetActorLocation()
			+ Character->GetActorForwardVector() * 600.f
			+ FVector(0.f, 0.f, 200.f);

		FHitResult GroundHit;
		FCollisionQueryParams Params(TEXT("CWTestSpawn"), false, Character);
		const FVector TraceStart = SpawnLocation;
		const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 1200.f);
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
		{
			SpawnLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, 150.f);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Vehicle = World->SpawnActor<ACWVehicleBase>(
			ACWVehicleBase::StaticClass(), SpawnLocation, Character->GetActorRotation(), SpawnParams);

		if (!Vehicle)
		{
			UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear un vehiculo de prueba."));
			return false;
		}

		UE_LOG(LogCWTest, Display, TEXT("CW_TEST: vehiculo de prueba creado en %s."), *SpawnLocation.ToString());
	}

	InitialCharacterCount = CountCharacters(World);
	InitialVehicleCount = CountVehicles(World);

	UE_LOG(LogCWTest, Display, TEXT("CW_TEST: vehiculo %s en %s."),
		*Vehicle->GetName(), *Vehicle->GetActorLocation().ToString());

	for (const UActorComponent* Component : Vehicle->GetComponents())
	{
		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{
			UE_LOG(LogCWTest, Verbose, TEXT("CW_TEST: componente %s loc=%s bounds=%s."),
				*Component->GetName(),
				*SceneComponent->GetComponentLocation().ToString(),
				*SceneComponent->Bounds.GetBox().ToString());
		}
	}

	return true;
}

bool ACWTestRunner::ValidateCameraSetup(ACWPlayerCharacter* InCharacter, FString& OutError) const
{
	if (!InCharacter)
	{
		OutError = TEXT("personaje nulo al validar la camara");
		return false;
	}

	const USpringArmComponent* Boom = InCharacter->FindComponentByClass<USpringArmComponent>();
	if (!Boom)
	{
		OutError = TEXT("el personaje no tiene SpringArm");
		return false;
	}

	if (!Boom->bDoCollisionTest)
	{
		OutError = TEXT("el SpringArm no tiene colision activada");
		return false;
	}

	if (Boom->ProbeSize < 8.f)
	{
		OutError = FString::Printf(TEXT("ProbeSize demasiado pequeno (%0.1f)"), Boom->ProbeSize);
		return false;
	}

	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		OutError = TEXT("no hay PlayerCameraManager");
		return false;
	}

	if (PlayerController->PlayerCameraManager->ViewPitchMin > -45.f
		|| PlayerController->PlayerCameraManager->ViewPitchMax < 45.f)
	{
		OutError = FString::Printf(
			TEXT("limites de pitch insuficientes [%0.1f, %0.1f]"),
			PlayerController->PlayerCameraManager->ViewPitchMin,
			PlayerController->PlayerCameraManager->ViewPitchMax);
		return false;
	}

	int32 CameraCount = 0;
	for (const UActorComponent* Component : InCharacter->GetComponents())
	{
		if (Cast<UCameraComponent>(Component))
		{
			++CameraCount;
		}
	}

	if (CameraCount < 2)
	{
		OutError = FString::Printf(TEXT("solo hay %d camara(s); se esperaban 2 (1a y 3a persona)"), CameraCount);
		return false;
	}

	return true;
}

void ACWTestRunner::RunEnterExitTest()
{
	if (bFinished)
	{
		return;
	}

	if (!EnsureActors())
	{
		FinishTest(false, TEXT("no se pudo preparar personaje/vehiculo"));
		return;
	}

	FString CameraError;
	if (!ValidateCameraSetup(Character, CameraError))
	{
		FinishTest(false, FString::Printf(TEXT("camara: %s"), *CameraError));
		return;
	}

	UWorld* World = GetWorld();
	// Solo la geometria con colision: la camara y el spring arm tienen bounds
	// mucho mayores y no representan el volumen fisico del vehiculo.
	const FBox VehicleBounds = Vehicle->GetComponentsBoundingBox(false);
	bool bPass = true;
	FString FailReason;

	for (int32 Cycle = 1; Cycle <= PendingCycles && bPass; ++Cycle)
	{
		if (!Vehicle->TryEnter(Character))
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: TryEnter devolvio false"), Cycle);
			break;
		}

		if (PlayerController->GetPawn() != Vehicle)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el controller no posee el vehiculo"), Cycle);
		}
		else if (Vehicle->GetDriver() != Character)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: Driver no es el personaje"), Cycle);
		}
		else if (!Character->IsHidden())
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje sigue visible dentro del vehiculo"), Cycle);
		}
		else if (Character->GetActorEnableCollision())
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje conserva colision dentro del vehiculo"), Cycle);
		}
		else if (Character->GetCharacterMovement()->MovementMode != MOVE_None)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje conserva movimiento dentro del vehiculo"), Cycle);
		}

		if (!bPass)
		{
			break;
		}

		Vehicle->ExitVehicle();

		if (PlayerController->GetPawn() != Character)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el controller no recupera el personaje"), Cycle);
		}
		else if (Vehicle->GetDriver() != nullptr)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: Driver no se limpio al salir"), Cycle);
		}
		else if (Character->IsHidden())
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje sigue oculto tras salir"), Cycle);
		}
		else if (!Character->GetActorEnableCollision())
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje no recupera colision"), Cycle);
		}
		else if (Character->GetCharacterMovement()->MovementMode != MOVE_Walking)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: el personaje no recupera MOVE_Walking"), Cycle);
		}
		else if (VehicleBounds.IsInsideOrOn(Character->GetActorLocation()))
		{
			bPass = false;
			FailReason = FString::Printf(
				TEXT("ciclo %d: el personaje sale dentro del vehiculo (char=%s, veh=%s, bounds=%s)"),
				Cycle,
				*Character->GetActorLocation().ToString(),
				*Vehicle->GetActorLocation().ToString(),
				*VehicleBounds.ToString());
		}
		else if (CountCharacters(World) != InitialCharacterCount)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: se duplicaron personajes (%d -> %d)"),
				Cycle, InitialCharacterCount, CountCharacters(World));
		}
		else if (CountVehicles(World) != InitialVehicleCount)
		{
			bPass = false;
			FailReason = FString::Printf(TEXT("ciclo %d: se duplicaron vehiculos (%d -> %d)"),
				Cycle, InitialVehicleCount, CountVehicles(World));
		}
		else
		{
			FHitResult GroundHit;
			FCollisionQueryParams Params(TEXT("CWTestGround"), false, Character);
			Params.AddIgnoredActor(Vehicle);
			const FVector Start = Character->GetActorLocation();
			const FVector End = Start - FVector(0.f, 0.f, 500.f);
			if (!World->LineTraceSingleByChannel(GroundHit, Start, End, ECC_WorldStatic, Params))
			{
				bPass = false;
				FailReason = FString::Printf(TEXT("ciclo %d: el personaje no tiene suelo bajo los pies"), Cycle);
			}
		}

		if (bPass)
		{
			UE_LOG(LogCWTest, Verbose, TEXT("CW_TEST: ciclo %d/%d OK."), Cycle, PendingCycles);
		}
	}

	if (bPass)
	{
		FinishTest(true, FString::Printf(
			TEXT("camara OK; %d ciclos sin duplicar actores ni perder controles"), PendingCycles));
	}
	else
	{
		FinishTest(false, FailReason);
	}
}

void ACWTestRunner::StartExcavatorStabilityTest(bool bInExitWhenDone)
{
	PendingCycles = 3;
	bExitWhenDone = bInExitWhenDone;
	bFinished = false;

	UE_LOG(LogCWTest, Display, TEXT("CW_TEST: programada la prueba de estabilidad de la excavadora."));

	GetWorldTimerManager().SetTimer(
		ExcavatorTimer, this, &ACWTestRunner::RunExcavatorStabilityTest, 1.5f, false);
}

bool ACWTestRunner::SpawnExcavatorForTest()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no hay mundo para la prueba de excavadora."));
		return false;
	}

	if (!PlayerController || !Character)
	{
		if (!EnsureActors())
		{
			return false;
		}
	}

	FVector SpawnLocation = Character->GetActorLocation()
		+ Character->GetActorForwardVector() * 1200.f
		+ FVector(0.f, 0.f, 300.f);

	FHitResult GroundHit;
	FCollisionQueryParams Params(TEXT("CWTestExcavatorSpawn"), false, Character);
	const FVector TraceStart = SpawnLocation;
	const FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 1600.f);
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		SpawnLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, 100.f);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Excavator = World->SpawnActor<ACWExcavator>(
		ACWExcavator::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (!Excavator)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear la excavadora de prueba."));
		return false;
	}

	Excavator->StartEngine();
	UE_LOG(LogCWTest, Display, TEXT("CW_TEST: excavadora de prueba en %s."),
		*SpawnLocation.ToString());
	return true;
}

void ACWTestRunner::RunExcavatorStabilityTest()
{
	if (bFinished)
	{
		return;
	}

	if (!SpawnExcavatorForTest())
	{
		FinishTest(false, TEXT("no se pudo preparar la excavadora"));
		return;
	}

	// Caso 1: brazo recogido, cuchara vacia, suelo plano.
	Excavator->SetBucketLoadForTest(0.f);
	Excavator->SetTestPose(0.f, 0.f, 0.f, 0.f);
	const float FlatEmptyMargin = Excavator->GetStabilityMarginCm(0.f, 0.f);

	// Caso 2: cuchara llena, suelo plano.
	Excavator->SetBucketLoadForTest(Excavator->GetBucketCapacityCubicMeters());
	const float FlatFullMargin = Excavator->GetStabilityMarginCm(0.f, 0.f);

	// Caso 3: torreta girada 90 grados, primero vacia y luego llena (peor caso lateral).
	Excavator->SetTestPose(90.f, 0.f, 0.f, 0.f);
	Excavator->SetBucketLoadForTest(0.f);
	const float SideEmptyMargin = Excavator->GetStabilityMarginCm(0.f, 0.f);

	Excavator->SetBucketLoadForTest(Excavator->GetBucketCapacityCubicMeters());
	const float SideFullMargin = Excavator->GetStabilityMarginCm(0.f, 0.f);

	// Caso 4: mismo caso lateral lleno sobre una pendiente de 30 grados.
	const float SideSlopeMargin = Excavator->GetStabilityMarginCm(30.f, 90.f);

	const float Safety = Excavator->SafetyMarginCm;
	const bool bFlatStable = FlatEmptyMargin >= Safety && FlatFullMargin >= Safety;
	const bool bLoadReducesMargin = SideFullMargin < SideEmptyMargin;
	const bool bSlopeCritical = SideSlopeMargin < Safety;
	const bool bPass = bFlatStable && bLoadReducesMargin && bSlopeCritical;

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST: estabilidad (cm) plana vacia=%.1f, plana llena=%.1f, lateral vacia=%.1f, "
		     "lateral llena=%.1f, lateral 30 grados=%.1f, seguridad=%.1f."),
		FlatEmptyMargin, FlatFullMargin, SideEmptyMargin, SideFullMargin, SideSlopeMargin, Safety);

	if (bPass)
	{
		FinishTest(true, FString::Printf(
			TEXT("estabilidad OK (plana %.1f/%.1f cm, carga reduce margen %.1f->%.1f cm, pendiente critica %.1f cm)"),
			FlatEmptyMargin, FlatFullMargin, SideEmptyMargin, SideFullMargin, SideSlopeMargin));
	}
	else
	{
		FinishTest(false, FString::Printf(
			TEXT("estabilidad fuera de rango (plana %.1f/%.1f cm, lateral %.1f/%.1f cm, pendiente %.1f cm, seguridad %.1f)"),
			FlatEmptyMargin, FlatFullMargin, SideEmptyMargin, SideFullMargin, SideSlopeMargin, Safety));
	}
}

void ACWTestRunner::StartTruckDumpTest(bool bInExitWhenDone)
{
	PendingCycles = 5;
	bExitWhenDone = bInExitWhenDone;
	bFinished = false;

	UE_LOG(LogCWTest, Display, TEXT("CW_TEST: programada la prueba de carga/descarga del volquete."));

	GetWorldTimerManager().SetTimer(
		TruckTimer, this, &ACWTestRunner::RunTruckDumpTest, 1.5f, false);
}

bool ACWTestRunner::SpawnTruckAndZoneForTest()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no hay mundo para la prueba del volquete."));
		return false;
	}

	if (!PlayerController || !Character)
	{
		if (!EnsureActors())
		{
			return false;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	DumpZone = World->SpawnActor<ACWDumpZone>(
		ACWDumpZone::StaticClass(), FVector(0.f, -2500.f, 250.f), FRotator::ZeroRotator, SpawnParams);

	Truck = World->SpawnActor<ACWDumpTruck>(
		ACWDumpTruck::StaticClass(), FVector(0.f, -2500.f, 100.f), FRotator::ZeroRotator, SpawnParams);

	if (!DumpZone || !Truck)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudieron crear el volquete o la zona de descarga."));
		return false;
	}

	return true;
}

void ACWTestRunner::RunTruckDumpTest()
{
	if (bFinished)
	{
		return;
	}

	if (!SpawnTruckAndZoneForTest())
	{
		FinishTest(false, TEXT("no se pudo preparar el volquete"));
		return;
	}

	UWorld* World = GetWorld();

	// Carga abstracta y peso (F2.12).
	const float Loaded = Truck->AddLoad(5.f);
	const float WeightKg = Truck->GetLoadWeightKg();

	// Descarga en zona con la caja levantada (F2.11/F2.13).
	Truck->SetBedAngleForTest(Truck->MaxBedAngle);
	const bool bDumped = Truck->TryDump();
	const float Delivered = DumpZone->GetDeliveredCubicMeters();
	const float Remaining = Truck->GetLoadCubicMeters();

	// La excavadora vuelve a cargar el camion (F2.12).
	Truck->SetBedAngleForTest(0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Excavator = World->SpawnActor<ACWExcavator>(
		ACWExcavator::StaticClass(),
		Truck->GetActorLocation() + FVector(-300.f, 0.f, -25.f),
		FRotator::ZeroRotator, SpawnParams);

	bool bLoaded = false;
	float TruckLoadAfter = 0.f;
	if (Excavator)
	{
		Excavator->SetTestPose(0.f, 0.f, 0.f, 0.f);
		Excavator->SetBucketLoadForTest(1.f);
		bLoaded = Excavator->DigOrLoadTruck();
		TruckLoadAfter = Truck->GetLoadCubicMeters();
	}

	const bool bPass =
		FMath::IsNearlyEqual(Loaded, 5.f)
		&& FMath::IsNearlyEqual(WeightKg, 8000.f)
		&& bDumped
		&& FMath::IsNearlyEqual(Delivered, 5.f)
		&& Remaining <= KINDA_SMALL_NUMBER
		&& bLoaded
		&& FMath::IsNearlyEqual(TruckLoadAfter, 1.f);

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST: volquete cargado=%.2f m3 (%.0f kg), descarga=%s (zona %.2f m3, resto %.2f), "
		     "recarga excavadora=%s (camion %.2f m3)."),
		Loaded, WeightKg, bDumped ? TEXT("OK") : TEXT("FALLO"), Delivered, Remaining,
		bLoaded ? TEXT("OK") : TEXT("FALLO"), TruckLoadAfter);

	if (bPass)
	{
		FinishTest(true, TEXT("volquete OK: carga, peso, descarga en zona y recarga desde excavadora"));
	}
	else
	{
		FinishTest(false, TEXT("volquete: fallo en carga, descarga o recarga"));
	}
}

void ACWTestRunner::StartTerrainTest(bool bInExitWhenDone)
{
	PendingCycles = 6;
	bExitWhenDone = bInExitWhenDone;
	bFinished = false;

	UE_LOG(LogCWTest, Display, TEXT("CW_TEST: programada la bateria de terreno (T-001..T-006)."));

	GetWorldTimerManager().SetTimer(
		TerrainTimer, this, &ACWTestRunner::RunTerrainTest, 1.0f, false);
}

bool ACWTestRunner::SpawnTerrainForTest()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no hay mundo para las pruebas de terreno."));
		return false;
	}

	auto SpawnField = [World](const FVector& Location, int32 Cells) -> ACWTerrainField*
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		ACWTerrainField* Field = World->SpawnActorDeferred<ACWTerrainField>(
			ACWTerrainField::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Field)
		{
			return nullptr;
		}

		Field->GridCellsX = Cells;
		Field->GridCellsY = Cells;
		Field->CellSizeCm = 100.f;
		Field->ChunkCells = 8;
		Field->MaxChunkRebuildsPerTick = 4;
		Field->bGenerateCollision = true;
		Field->bCarveInitialPit = false;
		Field->MinHeightCm = -600.f;
		Field->MaxHeightCm = 400.f;
		Field->FinishSpawning(Transform);
		return Field;
	};

	TerrainA = SpawnField(FVector(0.f, 20000.f, 0.f), 32);
	TerrainB = SpawnField(FVector(0.f, 40000.f, 0.f), 16);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Stockpile = World->SpawnActor<ACWStockpile>(
		ACWStockpile::StaticClass(), FVector(8000.f, 20000.f, 0.f),
		FRotator::ZeroRotator, Params);

	if (!TerrainA || !TerrainB || !Stockpile)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudieron crear los actores de terreno."));
		return false;
	}

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST: terreno A en %s (32x32, 32 m) y B en %s (16x16, 16 m)."),
		*TerrainA->GetActorLocation().ToString(), *TerrainB->GetActorLocation().ToString());
	return true;
}

void ACWTestRunner::RunTerrainTest()
{
	if (bFinished)
	{
		return;
	}

	if (!SpawnTerrainForTest())
	{
		FinishTest(false, TEXT("no se pudieron preparar los campos de terreno"));
		return;
	}

	const FVector CenterA = TerrainA->GetFieldCenter();
	const FVector CenterB = TerrainB->GetFieldCenter();

	// T-001: 10 cucharadas uniformes (1.2 m3) en posiciones separadas.
	float Sum1 = 0.f;
	float Min1 = TNumericLimits<float>::Max();
	float Max1 = 0.f;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Point = CenterA + FVector(-1200.f + Index * 260.f, 0.f, 0.f);
		const float Removed = TerrainA->DigAt(Point, 120.f, 120.f, 1.2f);
		Sum1 += Removed;
		Min1 = FMath::Min(Min1, Removed);
		Max1 = FMath::Max(Max1, Removed);
	}
	const bool bT1 = Min1 >= 0.8f && Max1 <= 1.4f && Sum1 >= 10.f && Sum1 <= 13.5f;

	// T-002: 100 cucharadas sobre la misma zona (debe topar con MinHeight).
	const FVector Point2 = CenterA + FVector(0.f, 600.f, 0.f);
	float Sum2 = 0.f;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Sum2 += TerrainA->DigAt(Point2, 150.f, 120.f, 1.2f);
	}
	const float Height2 = TerrainA->GetHeightAtLocation(Point2);
	const bool bT2 = !FMath::IsNaN(Height2)
		&& Height2 >= TerrainA->MinHeightCm - 1.f
		&& Sum2 >= 5.f
		&& Sum2 <= 140.f;

	// T-003: 1.000 operaciones distribuidas, con presupuesto de tiempo y conservacion.
	const float Initial3 = TerrainA->GetTotalVolumeM3();
	const double Start3 = FPlatformTime::Seconds();
	float Sum3 = 0.f;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		const int32 GridX = Index % 20;
		const int32 GridY = (Index / 20) % 20;
		const FVector Point = CenterA + FVector(-950.f + GridX * 100.f, -950.f + GridY * 100.f, 0.f);
		Sum3 += TerrainA->DigAt(Point, 100.f, 60.f, 0.5f);
	}
	const double Elapsed3 = FPlatformTime::Seconds() - Start3;
	const float Final3 = TerrainA->GetTotalVolumeM3();
	const float Conservation3 = FMath::Abs((Initial3 - Final3) - Sum3);
	const int32 PendingBefore = TerrainA->GetPendingChunkCount();
	const int32 Processed3 = TerrainA->FlushDirtyChunks(TerrainA->MaxChunkRebuildsPerTick);
	const int32 PendingAfter = TerrainA->GetPendingChunkCount();
	const bool bT3 = Elapsed3 < 5.0
		&& Conservation3 < 0.5f
		&& Sum3 > 0.f
		&& Processed3 <= TerrainA->MaxChunkRebuildsPerTick
		&& PendingAfter <= PendingBefore;

	// T-004: excavar y volver a depositar conservando el volumen.
	const float Initial4 = TerrainB->GetTotalVolumeM3();
	const FVector Point4 = CenterB;
	const float Removed4 = TerrainB->DigAt(Point4, 300.f, 100.f, 20.f);
	const float Deposited4 = TerrainB->DepositAt(Point4, 300.f, Removed4, ECWSoilType::Earth);
	const float Final4 = TerrainB->GetTotalVolumeM3();
	const float Tolerance4 = FMath::Max(0.2f, Removed4 * 0.02f);
	const bool bT4 = Removed4 > 5.f
		&& Deposited4 > 0.f
		&& FMath::Abs(Final4 - Initial4) <= Tolerance4;

	// T-005: guardar/cargar deltas y rechazar archivos corruptos o truncados.
	const FString SavePath = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("Terrain"), TEXT("test_terrain.cwt"));
	FString SaveError;
	const float BeforeSave = TerrainA->GetTotalVolumeM3();
	const double SaveStart = FPlatformTime::Seconds();
	const bool bSaved = TerrainA->SaveStateToFile(SavePath, SaveError);
	const double SaveMs = (FPlatformTime::Seconds() - SaveStart) * 1000.0;

	TerrainA->DigAt(CenterA + FVector(1300.f, 1300.f, 0.f), 150.f, 120.f, 2.f);
	const float Modified = TerrainA->GetTotalVolumeM3();

	FString LoadError;
	const double LoadStart = FPlatformTime::Seconds();
	const bool bLoaded = TerrainA->LoadStateFromFile(SavePath, LoadError);
	const double LoadMs = (FPlatformTime::Seconds() - LoadStart) * 1000.0;
	const float Restored = TerrainA->GetTotalVolumeM3();

	TArray<uint8> Garbage;
	Garbage.SetNumZeroed(128);
	const FString BadPath = SavePath + TEXT(".bad");
	FFileHelper::SaveArrayToFile(Garbage, *BadPath);
	FString BadError;
	const bool bBadLoaded = TerrainA->LoadStateFromFile(BadPath, BadError);

	TArray<uint8> ValidData;
	FFileHelper::LoadFileToArray(ValidData, *SavePath);
	ValidData.SetNum(ValidData.Num() / 2);
	const FString TruncPath = SavePath + TEXT(".trunc");
	FFileHelper::SaveArrayToFile(ValidData, *TruncPath);
	FString TruncError;
	const bool bTruncLoaded = TerrainA->LoadStateFromFile(TruncPath, TruncError);

	const bool bT5 = bSaved && bLoaded
		&& FMath::Abs(Restored - BeforeSave) <= 0.1f
		&& FMath::Abs(Modified - BeforeSave) > 0.5f
		&& !bBadLoaded && !BadError.IsEmpty()
		&& !bTruncLoaded && !TruncError.IsEmpty();

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST T5 detalle: saved=%d loaded=%d badLoaded=%d truncLoaded=%d | ")
		TEXT("antes=%.3f modificado=%.3f restaurado=%.3f | saveErr='%s' loadErr='%s' badErr='%s' truncErr='%s'"),
		bSaved, bLoaded, bBadLoaded, bTruncLoaded,
		BeforeSave, Modified, Restored,
		*SaveError, *LoadError, *BadError, *TruncError);

	// T-006: cambio de material y densidad.
	const FVector Point6 = CenterA + FVector(1200.f, -1200.f, 0.f);
	const bool bSetSoil = TerrainA->SetSoilTypeInArea(Point6, 600.f, ECWSoilType::Gravel);
	const ECWSoilType Type6 = TerrainA->GetSoilTypeAtLocation(Point6);
	const float Removed6 = TerrainA->DigAt(Point6, 120.f, 120.f, 1.f);
	const float GravelDensity = CWSoil::GetDensityKgPerCubicMeter(ECWSoilType::Gravel);
	const bool bT6 = bSetSoil
		&& Type6 == ECWSoilType::Gravel
		&& Removed6 > 0.5f
		&& FMath::IsNearlyEqual(GravelDensity, 1800.f);

	// F3.08: acopio con volumen y masa.
	const float AddedPile = Stockpile->AddMaterial(5.f);
	const float RemovedPile = Stockpile->RemoveMaterial(2.f);
	const bool bStockpile = FMath::IsNearlyEqual(AddedPile, 5.f)
		&& FMath::IsNearlyEqual(RemovedPile, 2.f)
		&& FMath::IsNearlyEqual(Stockpile->GetVolumeM3(), 3.f)
		&& FMath::IsNearlyEqual(Stockpile->GetMassKg(), 3.f * 1600.f, 1.f);

	// F3.04: benchmark de memoria y tiempos.
	const int64 MemoryA = TerrainA->GetApproxMemoryBytes();
	const int64 MemoryTarget = MemoryA * 4; // extrapolacion 32x32 -> 64x64
	const bool bBenchmark = MemoryTarget < 8 * 1024 * 1024
		&& SaveMs < 1000.0
		&& LoadMs < 1000.0
		&& Elapsed3 < 5.0;

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TEST: T1 cucharadas sum=%.2f m3 (%.2f..%.2f); T2 100 ops sum=%.2f m3 altura=%.1f cm; "
		     "T3 1000 ops=%.0f ms conservacion=%.3f m3 chunks %d->%d; "
		     "T4 excavado=%.2f depositado=%.2f final=%.2f; "
		     "T5 guardado=%.1f ms carga=%.1f ms restaurado=%.2f; T6 material=%s densidad=%.0f; acopio=%.1f m3."),
		Sum1, Min1, Max1, Sum2, Height2,
		Elapsed3 * 1000.0, Conservation3, PendingBefore, PendingAfter,
		Removed4, Deposited4, Final4,
		SaveMs, LoadMs, Restored,
		*CWSoil::GetSoilProperties(Type6).DisplayName, GravelDensity, Stockpile->GetVolumeM3());

	UE_LOG(LogCWTest, Display,
		TEXT("CW_TERRAIN_BENCHMARK: memoria 32x32=%.1f KB, extrapolada 64x64=%.1f KB, "
		     "save=%.1f ms, load=%.1f ms, 1000 digs=%.1f ms (%.3f ms/op), rebuilds/tick=%d."),
		MemoryA / 1024.0, MemoryTarget / 1024.0, SaveMs, LoadMs,
		Elapsed3 * 1000.0, Elapsed3, TerrainA->MaxChunkRebuildsPerTick);

	const bool bPass = bT1 && bT2 && bT3 && bT4 && bT5 && bT6 && bStockpile && bBenchmark;

	if (bPass)
	{
		FinishTest(true, FString::Printf(
			TEXT("terreno OK: T1 %.1f m3, T2 tope %.1f cm, T3 %.0f ms/1000 ops, T4 conserva %.2f m3, "
			     "T5 guardado/carga y corrupcion, T6 grava, acopio %.1f m3"),
			Sum1, Height2, Elapsed3 * 1000.0, Final4, Stockpile->GetVolumeM3()));
	}
	else
	{
		FinishTest(false, FString::Printf(
			TEXT("terreno: fallo T1=%d T2=%d T3=%d T4=%d T5=%d T6=%d acopio=%d benchmark=%d"),
			bT1, bT2, bT3, bT4, bT5, bT6, bStockpile, bBenchmark));
	}
}

void ACWTestRunner::FinishTest(bool bPass, const FString& Summary)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;

	if (bPass)
	{
		UE_LOG(LogCWTest, Display, TEXT("CW_TEST_RESULT: PASS cycles=%d %s"), PendingCycles, *Summary);
	}
	else
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST_RESULT: FAIL cycles=%d %s"), PendingCycles, *Summary);
	}

	if (bExitWhenDone)
	{
		FPlatformMisc::RequestExitWithStatus(true, bPass ? 0 : 1);
	}
	else
	{
		SetLifeSpan(2.f);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GCWTestEnterExit(
	TEXT("cw.Test.EnterExit"),
	TEXT("Ejecuta N ciclos de entrar/salir de un vehiculo y valida el estado. Uso: cw.Test.EnterExit 10"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: comando sin mundo."));
				return;
			}

			const int32 Cycles = Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 10;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWTestRunner* Runner = World->SpawnActor<ACWTestRunner>(
				ACWTestRunner::StaticClass(), FTransform::Identity, Params);

			if (!Runner)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear el ejecutor de pruebas."));
				return;
			}

			Runner->StartEnterExitTest(Cycles, 0.5f, false);
		}));

static FAutoConsoleCommandWithWorldAndArgs GCWTestExcavator(
	TEXT("cw.Test.Excavator"),
	TEXT("Ejecuta la prueba de estabilidad de la excavadora (suelo plano, carga y pendiente)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: comando sin mundo."));
				return;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWTestRunner* Runner = World->SpawnActor<ACWTestRunner>(
				ACWTestRunner::StaticClass(), FTransform::Identity, Params);

			if (!Runner)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear el ejecutor de pruebas."));
				return;
			}

			Runner->StartExcavatorStabilityTest(false);
		}));

static FAutoConsoleCommandWithWorldAndArgs GCWTestTruck(
	TEXT("cw.Test.Truck"),
	TEXT("Ejecuta la prueba del volquete: carga, peso, descarga en zona y recarga desde excavadora."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: comando sin mundo."));
				return;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWTestRunner* Runner = World->SpawnActor<ACWTestRunner>(
				ACWTestRunner::StaticClass(), FTransform::Identity, Params);

			if (!Runner)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear el ejecutor de pruebas."));
				return;
			}

			Runner->StartTruckDumpTest(false);
		}));

static FAutoConsoleCommandWithWorldAndArgs GCWTestTerrain(
	TEXT("cw.Test.Terrain"),
	TEXT("Ejecuta la bateria T-001..T-006 del terreno deformable (excavacion, volumen, guardado y materiales)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: comando sin mundo."));
				return;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWTestRunner* Runner = World->SpawnActor<ACWTestRunner>(
				ACWTestRunner::StaticClass(), FTransform::Identity, Params);

			if (!Runner)
			{
				UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear el ejecutor de pruebas."));
				return;
			}

			Runner->StartTerrainTest(false);
		}));
