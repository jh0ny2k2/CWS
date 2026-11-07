#include "CWDumpZone.h"

#include "CWLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACWDumpZone::ACWDumpZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
	SetRootComponent(ZoneBox);
	ZoneBox->SetBoxExtent(FVector(700.f, 700.f, 250.f));
	ZoneBox->SetCollisionProfileName(TEXT("Trigger"));
	ZoneBox->SetGenerateOverlapEvents(true);

	// Marca visual plana en el suelo (sin colision).
	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(ZoneBox);
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, -245.f));
	MarkerMesh->SetRelativeScale3D(FVector(14.f, 14.f, 0.1f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ACWDumpZone::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCW, Log, TEXT("CWDumpZone '%s' lista (capacidad %.0f m3)."),
		*ZoneName, CapacityCubicMeters);
}

bool ACWDumpZone::DeliverLoad(float CubicMeters, AActor* Source)
{
	if (CubicMeters <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float FreeSpace = FMath::Max(0.f, CapacityCubicMeters - DeliveredCubicMeters);
	if (FreeSpace <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogCW, Warning, TEXT("CWDumpZone '%s': la zona esta llena."), *ZoneName);
		return false;
	}

	const float Accepted = FMath::Min(CubicMeters, FreeSpace);
	DeliveredCubicMeters += Accepted;

	UE_LOG(LogCW, Log,
		TEXT("CWDumpZone '%s': recibidos %.2f m3 de %s (total %.2f / %.0f m3)."),
		*ZoneName, Accepted, Source ? *Source->GetName() : TEXT("desconocido"),
		DeliveredCubicMeters, CapacityCubicMeters);

	OnLoadDelivered.Broadcast(Accepted, Source);
	return true;
}

bool ACWDumpZone::IsActorInside(const AActor* Actor) const
{
	if (!Actor || !ZoneBox)
	{
		return false;
	}

	const FBox Box = ZoneBox->Bounds.GetBox();
	return Box.IsInsideOrOn(Actor->GetActorLocation());
}

FText ACWDumpZone::GetZoneStatusText() const
{
	return FText::FromString(FString::Printf(
		TEXT("%s: %.1f / %.0f m3"), *ZoneName, DeliveredCubicMeters, CapacityCubicMeters));
}
