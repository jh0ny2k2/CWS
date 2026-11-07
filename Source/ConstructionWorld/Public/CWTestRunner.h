#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWTestRunner.generated.h"

class ACWPlayerCharacter;
class ACWVehicleBase;
class ACWExcavator;
class ACWDumpTruck;
class ACWDumpZone;
class APlayerController;

/**
 * Ejecutor de pruebas funcionales (F1.07).
 *
 * Prueba el ciclo completo de entrar/salir de un vehiculo N veces y valida:
 *  - La posesion cambia al vehiculo y vuelve al personaje.
 *  - No se duplican personajes ni vehiculos.
 *  - El personaje recupera visibilidad, colision y movimiento Walking.
 *  - El punto de salida no queda dentro del vehiculo.
 *  - La camara tiene colision y limites de pitch configurados.
 *
 * Uso en PIE (consola):
 *   cw.Test.EnterExit 10
 *
 * Uso headless (CI/local):
 *   UnrealEditor-Cmd.exe ConstructionWorld.uproject /Game/Maps/TestLevel -game -nullrhi \
 *     -CWTestEnterExit=10 -CWTestExit -unattended -nosplash -log
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWTestRunner : public AActor
{
	GENERATED_BODY()

public:
	ACWTestRunner();

	/** Programa la prueba tras un retardo para que el mundo y el jugador esten listos. */
	void StartEnterExitTest(int32 Cycles, float DelaySeconds, bool bExitWhenDone);

	/** Programa la prueba de estabilidad de la excavadora (F2.08). */
	void StartExcavatorStabilityTest(bool bExitWhenDone);

	/** Programa la prueba de carga/descarga del volquete (F2.10-F2.13). */
	void StartTruckDumpTest(bool bExitWhenDone);

private:
	void RunEnterExitTest();
	void RunExcavatorStabilityTest();
	void RunTruckDumpTest();
	bool SpawnExcavatorForTest();
	bool SpawnTruckAndZoneForTest();
	bool EnsureActors();
	bool ValidateCameraSetup(ACWPlayerCharacter* InCharacter, FString& OutError) const;
	void FinishTest(bool bPass, const FString& Summary);

	UPROPERTY(Transient)
	TObjectPtr<ACWPlayerCharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<ACWVehicleBase> Vehicle;

	UPROPERTY(Transient)
	TObjectPtr<ACWExcavator> Excavator;

	UPROPERTY(Transient)
	TObjectPtr<ACWDumpTruck> Truck;

	UPROPERTY(Transient)
	TObjectPtr<ACWDumpZone> DumpZone;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> PlayerController;

	FTimerHandle StartTimer;
	FTimerHandle ExcavatorTimer;
	FTimerHandle TruckTimer;

	int32 PendingCycles = 0;
	int32 InitialCharacterCount = 0;
	int32 InitialVehicleCount = 0;
	bool bExitWhenDone = false;
	bool bFinished = false;
};
