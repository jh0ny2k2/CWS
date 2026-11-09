#include "CWGameMode.h"

#include "CWHUD.h"
#include "CWPlayerCharacter.h"
#include "CWWeatherSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ACWGameMode::ACWGameMode()
{
	DefaultPawnClass = ACWPlayerCharacter::StaticClass();
	HUDClass = ACWHUD::StaticClass();
}

void ACWGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Sistema de clima de H4 (F5.07). Se crea apagado; se controla con
	// cw.Weather.Rain 1 / 0 desde la consola.
	for (TActorIterator<ACWWeatherSystem> It(World); It; ++It)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ACWWeatherSystem>(
		ACWWeatherSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
