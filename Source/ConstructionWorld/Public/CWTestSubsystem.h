#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWTestSubsystem.generated.h"

/**
 * Arranca pruebas funcionales automaticas por linea de comandos (F1.07).
 *
 * Si el proceso se lanza con `-CWTestEnterExit=N`, al empezar el mundo de juego
 * crea un ACWTestRunner y ejecuta N ciclos de entrar/salir. Con `-CWTestExit`
 * el proceso termina con codigo 0 (PASS) o 1 (FAIL), lo que permite usarlo en CI.
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWTestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
