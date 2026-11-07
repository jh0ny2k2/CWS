#include "CWStockpile.h"

#include "CWLog.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACWStockpile::ACWStockpile()
{
	PrimaryActorTick.bCanEverTick = false;

	PileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PileMesh"));
	SetRootComponent(PileMesh);
	PileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PileMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		PileMesh->SetStaticMesh(ConeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		PileMaterialAsset = DefaultMaterial.Object;
	}
}

void ACWStockpile::BeginPlay()
{
	Super::BeginPlay();

	if (PileMaterialAsset)
	{
		PileMaterial = UMaterialInstanceDynamic::Create(PileMaterialAsset, this);
		if (PileMaterial)
		{
			PileMesh->SetMaterial(0, PileMaterial);
		}
	}
	else
	{
		PileMesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
	}

	UpdateVisual();
}

float ACWStockpile::AddMaterial(float CubicMeters)
{
	if (CubicMeters <= 0.f)
	{
		return 0.f;
	}

	const float FreeSpace = FMath::Max(0.f, MaxVolumeM3 - VolumeM3);
	const float Accepted = FMath::Min(CubicMeters, FreeSpace);
	VolumeM3 += Accepted;

	if (Accepted > KINDA_SMALL_NUMBER)
	{
		UpdateVisual();
		UE_LOG(LogCWTerrain, Log, TEXT("CWStockpile '%s': +%.2f m3 (total %.2f m3, %.0f kg)."),
			*GetName(), Accepted, VolumeM3, GetMassKg());
	}

	return Accepted;
}

float ACWStockpile::RemoveMaterial(float CubicMeters)
{
	if (CubicMeters <= 0.f)
	{
		return 0.f;
	}

	const float Removed = FMath::Min(CubicMeters, VolumeM3);
	VolumeM3 -= Removed;

	if (Removed > KINDA_SMALL_NUMBER)
	{
		UpdateVisual();
		UE_LOG(LogCWTerrain, Log, TEXT("CWStockpile '%s': -%.2f m3 (total %.2f m3)."),
			*GetName(), Removed, VolumeM3);
	}

	return Removed;
}

void ACWStockpile::SetSoilType(ECWSoilType NewSoilType)
{
	SoilType = NewSoilType;
	UpdateVisual();
}

float ACWStockpile::GetMassKg() const
{
	return VolumeM3 * CWSoil::GetDensityKgPerCubicMeter(SoilType);
}

FVector ACWStockpile::GetPileCenter() const
{
	return PileMesh ? PileMesh->Bounds.Origin : GetActorLocation();
}

void ACWStockpile::UpdateVisual()
{
	if (!PileMesh)
	{
		return;
	}

	const float Ratio = MaxVolumeM3 > 0.f ? FMath::Clamp(VolumeM3 / MaxVolumeM3, 0.f, 1.f) : 0.f;
	const float HeightScale = FMath::Max(0.01f, Ratio);
	const float RadiusScale = FMath::Max(0.01f, FMath::Sqrt(Ratio)) * (BaseRadiusCm / 50.f);

	PileMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, HeightScale));
	PileMesh->SetRelativeLocation(FVector(0.f, 0.f, 50.f * HeightScale));
	PileMesh->SetVisibility(VolumeM3 > KINDA_SMALL_NUMBER);

	if (PileMaterial)
	{
		PileMaterial->SetVectorParameterValue(
			TEXT("Color"), CWSoil::GetSoilProperties(SoilType).Color);
	}
}
