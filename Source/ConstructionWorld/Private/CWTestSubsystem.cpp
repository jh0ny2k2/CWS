#include "CWTestSubsystem.h"

#include "CWLog.h"
#include "CWTestRunner.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UCWTestSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	int32 Cycles = 0;
	if (!FParse::Value(FCommandLine::Get(), TEXT("CWTestEnterExit="), Cycles) || Cycles <= 0)
	{
		return;
	}

	const bool bExitWhenDone = FParse::Param(FCommandLine::Get(), TEXT("CWTestExit"));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACWTestRunner* Runner = InWorld.SpawnActor<ACWTestRunner>(
		ACWTestRunner::StaticClass(), FTransform::Identity, Params);

	if (!Runner)
	{
		UE_LOG(LogCWTest, Error, TEXT("CW_TEST: no se pudo crear el ejecutor automatico."));
		return;
	}

	Runner->StartEnterExitTest(Cycles, 1.5f, bExitWhenDone);
}
