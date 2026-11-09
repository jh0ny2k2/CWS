#include "CWWeatherSystem.h"

#include "CWLog.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* const RainDropMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const RainMaterialPath =
		TEXT("/Game/ConstructionWorld/Materials/M_CW_TranslucentFX.M_CW_TranslucentFX");

	ACWWeatherSystem* FindWeatherSystem(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<ACWWeatherSystem> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	void CWWeatherRainCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		bool bEnable = true;
		if (Args.Num() > 0)
		{
			bEnable = Args[0] != TEXT("0") && !Args[0].Equals(TEXT("false"), ESearchCase::IgnoreCase);
		}

		ACWWeatherSystem* Weather = FindWeatherSystem(World);
		if (!Weather)
		{
			if (!bEnable)
			{
				return;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Weather = World->SpawnActor<ACWWeatherSystem>(
				ACWWeatherSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		}

		if (Weather)
		{
			Weather->SetRainEnabled(bEnable);
		}
	}

	void CWWeatherStatusCommand(const TArray<FString>& Args, UWorld* World)
	{
		ACWWeatherSystem* Weather = FindWeatherSystem(World);
		const FString Message = Weather
			? FString::Printf(TEXT("cw.Weather: lluvia %s, %d gotas"),
				Weather->IsRainEnabled() ? TEXT("ON") : TEXT("OFF"), Weather->MaxDrops)
			: FString(TEXT("cw.Weather: sistema no creado (se crea al activar la lluvia)"));

		UE_LOG(LogCW, Log, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Message);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GWeatherRain(
		TEXT("cw.Weather.Rain"),
		TEXT("Activa (1) o desactiva (0) la lluvia y el ambiente humedo de H4."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CWWeatherRainCommand));

	FAutoConsoleCommandWithWorldAndArgs GWeatherStatus(
		TEXT("cw.Weather.Status"),
		TEXT("Muestra el estado del sistema de clima."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CWWeatherStatusCommand));
}

ACWWeatherSystem::ACWWeatherSystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	RainInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainInstances"));
	RainInstances->SetupAttachment(Root);
	RainInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainInstances->SetGenerateOverlapEvents(false);
	RainInstances->SetCastShadow(false);
	RainInstances->SetVisibility(false);
	RainInstances->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DropFinder(RainDropMeshPath);
	if (DropFinder.Succeeded())
	{
		DropMesh = DropFinder.Object;
		RainInstances->SetStaticMesh(DropMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(RainMaterialPath);
	if (MaterialFinder.Succeeded())
	{
		DropMaterial = MaterialFinder.Object;
	}
}

void ACWWeatherSystem::BeginPlay()
{
	Super::BeginPlay();

	CaptureAtmosphere();

	if (DropMaterial)
	{
		DropMID = UMaterialInstanceDynamic::Create(DropMaterial, this);
		if (DropMID)
		{
			DropMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.68f, 0.82f, 1.f));
			DropMID->SetScalarParameterValue(TEXT("Opacity"), 0.38f);
			RainInstances->SetMaterial(0, DropMID);
		}
	}

	// La lluvia empieza apagada: el nivel se ve de tarde y el usuario puede
	// probar el clima con cw.Weather.Rain 1.
	SetRainEnabled(false);
}

void ACWWeatherSystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRainEnabled)
	{
		UpdateDrops(DeltaSeconds);
	}
}

void ACWWeatherSystem::CaptureAtmosphere()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		SunLight = *It;
		if (const UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			BaseSunIntensity = Comp->Intensity;
		}
		break;
	}

	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		SkyLightActor = *It;
		if (const USkyLightComponent* Comp = Cast<USkyLightComponent>(It->GetLightComponent()))
		{
			BaseSkyIntensity = Comp->Intensity;
		}
		break;
	}

	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		HeightFog = *It;
		if (const UExponentialHeightFogComponent* Comp = It->GetComponent())
		{
			BaseFogDensity = Comp->FogDensity;
		}
		break;
	}

	bAtmosphereCaptured = true;
}

void ACWWeatherSystem::SetRainEnabled(bool bEnabled)
{
	if (bRainEnabled == bEnabled)
	{
		if (bEnabled)
		{
			RebuildDrops();
		}
		return;
	}

	bRainEnabled = bEnabled;
	RainInstances->SetVisibility(bRainEnabled);

	if (bRainEnabled)
	{
		RebuildDrops();
	}
	else
	{
		RainInstances->ClearInstances();
		DropOffsets.Reset();
	}

	ApplyWetLook(bRainEnabled);

	UE_LOG(LogCW, Log, TEXT("CWWeather: lluvia %s"), bRainEnabled ? TEXT("ON") : TEXT("OFF"));
}

void ACWWeatherSystem::ApplyWetLook(bool bWet)
{
	if (!bAtmosphereCaptured)
	{
		CaptureAtmosphere();
	}

	if (SunLight.IsValid())
	{
		if (UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
		{
			Comp->SetIntensity(bWet ? BaseSunIntensity * 0.32f : BaseSunIntensity);
			Comp->SetLightColor(bWet
				? FLinearColor(0.62f, 0.72f, 0.90f)
				: FLinearColor(1.f, 0.82f, 0.62f));
			Comp->MarkRenderStateDirty();
		}
	}

	if (SkyLightActor.IsValid())
	{
		if (USkyLightComponent* Comp = SkyLightActor->GetLightComponent())
		{
			Comp->SetIntensity(bWet ? BaseSkyIntensity * 0.65f : BaseSkyIntensity);
			Comp->MarkRenderStateDirty();
		}
	}

	if (HeightFog.IsValid())
	{
		if (UExponentialHeightFogComponent* Comp = HeightFog->GetComponent())
		{
			Comp->SetFogDensity(bWet ? BaseFogDensity * 2.4f : BaseFogDensity);
			Comp->SetVolumetricFog(true);
			Comp->SetVolumetricFogExtinctionScale(bWet ? 2.2f : 1.f);
		}
	}
}

void ACWWeatherSystem::RebuildDrops()
{
	RainInstances->ClearInstances();
	DropOffsets.Reset();

	if (!DropMesh)
	{
		return;
	}

	const int32 DropCount = FMath::Clamp(MaxDrops, 16, 4000);
	DropOffsets.Reserve(DropCount);

	const float RadiusSq = FMath::Square(RainRadiusCm);
	for (int32 Index = 0; Index < DropCount; ++Index)
	{
		FVector Offset;
		do
		{
			Offset.X = FMath::FRandRange(-RainRadiusCm, RainRadiusCm);
			Offset.Y = FMath::FRandRange(-RainRadiusCm, RainRadiusCm);
		}
		while (Offset.X * Offset.X + Offset.Y * Offset.Y > RadiusSq);

		Offset.Z = FMath::FRandRange(-350.f, RainHeightCm);
		DropOffsets.Add(Offset);

		RainInstances->AddInstance(FTransform::Identity, true);
	}

	for (int32 Index = 0; Index < DropOffsets.Num(); ++Index)
	{
		UpdateDropTransform(Index);
	}
}

void ACWWeatherSystem::UpdateDrops(float DeltaSeconds)
{
	for (int32 Index = 0; Index < DropOffsets.Num(); ++Index)
	{
		FVector& Offset = DropOffsets[Index];
		Offset += WindCmS * DeltaSeconds;
		Offset.Z -= FallSpeedCmS * DeltaSeconds;

		if (Offset.Z < -450.f)
		{
			Offset.Z = FMath::FRandRange(RainHeightCm * 0.75f, RainHeightCm);
			Offset.X = FMath::FRandRange(-RainRadiusCm, RainRadiusCm);
			Offset.Y = FMath::FRandRange(-RainRadiusCm, RainRadiusCm);
		}

		// Mantener las gotas dentro del volumen si el viento las desplaza.
		if (FMath::Abs(Offset.X) > RainRadiusCm)
		{
			Offset.X *= -0.9f;
		}
		if (FMath::Abs(Offset.Y) > RainRadiusCm)
		{
			Offset.Y *= -0.9f;
		}

		UpdateDropTransform(Index);
	}
}

void ACWWeatherSystem::UpdateDropTransform(int32 Index)
{
	if (!RainInstances || !DropOffsets.IsValidIndex(Index))
	{
		return;
	}

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector Center = CameraManager
		? CameraManager->GetCameraLocation()
		: GetActorLocation();

	const FVector WorldLocation = Center + DropOffsets[Index];
	const FTransform Transform(
		FRotator(0.f, 0.f, 14.f),
		WorldLocation,
		FVector(0.018f, 0.018f, 0.55f));

	RainInstances->UpdateInstanceTransform(Index, Transform, true, false, true);
}
