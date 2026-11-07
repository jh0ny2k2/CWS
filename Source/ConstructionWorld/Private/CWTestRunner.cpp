#include "CWTestRunner.h"

#include "CWLog.h"
#include "CWPlayerCharacter.h"
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
