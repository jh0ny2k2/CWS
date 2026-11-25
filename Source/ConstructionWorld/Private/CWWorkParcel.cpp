#include "CWWorkParcel.h"

#include "CWLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 StakeCount = 8;   // 4 esquinas + 4 puntos medios
	constexpr float StakeHeightCm = 120.f;
	constexpr float TapeHeightCm = 95.f;
}

ACWWorkParcel::ACWWorkParcel()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaBox"));
	SetRootComponent(AreaBox);
	AreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaBox->SetBoxExtent(FVector(HalfSizeCm.X, HalfSizeCm.Y, 200.f));
	AreaBox->ShapeColor = FColor(255, 140, 0);
	AreaBox->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ConeMat(
		TEXT("/Game/ConstructionWorld/Materials/MI_CW_ConeOrange.MI_CW_ConeOrange"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodMat(
		TEXT("/Game/ConstructionWorld/Materials/MI_CW_Wood.MI_CW_Wood"));

	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
	UStaticMesh* Cylinder = CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr;
	UMaterialInterface* Orange = ConeMat.Succeeded() ? ConeMat.Object : nullptr;
	UMaterialInterface* Wood = WoodMat.Succeeded() ? WoodMat.Object : nullptr;

	auto MakePart = [this](const FName Name, UStaticMesh* Mesh, UMaterialInterface* Material)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(AreaBox);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		if (Mesh)
		{
			Part->SetStaticMesh(Mesh);
		}
		if (Material)
		{
			Part->SetMaterial(0, Material);
		}
		return Part;
	};

	for (int32 Index = 0; Index < StakeCount; ++Index)
	{
		Stakes.Add(MakePart(*FString::Printf(TEXT("Stake_%d"), Index), Cylinder, Orange));
	}
	for (int32 Index = 0; Index < StakeCount; ++Index)
	{
		Tapes.Add(MakePart(*FString::Printf(TEXT("Tape_%d"), Index), Cube, Orange));
	}

	SignPost = MakePart(TEXT("SignPost"), Cube, Wood);
	SignBoard = MakePart(TEXT("SignBoard"), Cube, Wood);

	SignText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignText"));
	SignText->SetupAttachment(AreaBox);
	SignText->SetHorizontalAlignment(EHTA_Center);
	SignText->SetVerticalAlignment(EVRTA_TextCenter);
	SignText->SetWorldSize(22.f);
	SignText->SetTextRenderColor(FColor(25, 25, 25));
}

void ACWWorkParcel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildMarkers();
}

void ACWWorkParcel::BeginPlay()
{
	Super::BeginPlay();
	RebuildMarkers();
	UE_LOG(LogCWContract, Log, TEXT("Parcela %s en %s (%.0f m2)."),
		*ContractId, *GetActorLocation().ToCompactString(), GetAreaM2());
}

bool ACWWorkParcel::ContainsLocation(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(WorldLocation);
	return FMath::Abs(Local.X) <= HalfSizeCm.X + EdgeToleranceCm
		&& FMath::Abs(Local.Y) <= HalfSizeCm.Y + EdgeToleranceCm;
}

void ACWWorkParcel::RebuildMarkers()
{
	AreaBox->SetBoxExtent(FVector(HalfSizeCm.X, HalfSizeCm.Y, 200.f));

	// Perimetro en orden: esquinas y puntos medios alternados.
	const float X = HalfSizeCm.X;
	const float Y = HalfSizeCm.Y;
	const FVector2D Points[StakeCount] = {
		{ X, -Y }, { X, 0.f }, { X, Y }, { 0.f, Y }, { -X, Y }, { -X, 0.f }, { -X, -Y }, { 0.f, -Y } };

	// Altura del suelo bajo cada estaca, para que queden apoyadas aunque el terreno no sea plano.
	float GroundZ[StakeCount];
	const UWorld* World = GetWorld();
	for (int32 Index = 0; Index < StakeCount; ++Index)
	{
		GroundZ[Index] = 0.f;
		if (World && World->IsGameWorld())
		{
			const FVector WorldPoint = GetActorTransform().TransformPositionNoScale(FVector(Points[Index], 0.f));
			FHitResult Hit;
			FCollisionQueryParams Params(TEXT("CWParcelStake"), false, this);
			if (World->LineTraceSingleByChannel(Hit, WorldPoint + FVector(0.f, 0.f, 1000.f),
				WorldPoint - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic, Params))
			{
				GroundZ[Index] = Hit.ImpactPoint.Z - GetActorLocation().Z;
			}
		}
	}

	for (int32 Index = 0; Index < StakeCount; ++Index)
	{
		if (UStaticMeshComponent* Stake = Stakes.IsValidIndex(Index) ? Stakes[Index].Get() : nullptr)
		{
			Stake->SetRelativeLocation(FVector(Points[Index], GroundZ[Index] + StakeHeightCm * 0.5f));
			Stake->SetRelativeScale3D(FVector(0.07f, 0.07f, StakeHeightCm / 100.f));
		}

		const int32 Next = (Index + 1) % StakeCount;
		if (UStaticMeshComponent* Tape = Tapes.IsValidIndex(Index) ? Tapes[Index].Get() : nullptr)
		{
			const FVector A(Points[Index], GroundZ[Index] + TapeHeightCm);
			const FVector B(Points[Next], GroundZ[Next] + TapeHeightCm);
			const FVector Delta = B - A;
			Tape->SetRelativeLocation((A + B) * 0.5f);
			Tape->SetRelativeRotation(Delta.Rotation());
			Tape->SetRelativeScale3D(FVector(Delta.Size() / 100.f, 0.015f, 0.06f));
		}
	}

	// Cartel junto a la esquina delantera izquierda, mirando hacia +X.
	const FVector SignBase(X + 80.f, -Y - 80.f, GroundZ[0]);
	SignPost->SetRelativeLocation(SignBase + FVector(0.f, 0.f, 90.f));
	SignPost->SetRelativeScale3D(FVector(0.08f, 0.08f, 1.8f));
	SignBoard->SetRelativeLocation(SignBase + FVector(0.f, 0.f, 175.f));
	SignBoard->SetRelativeScale3D(FVector(0.05f, 1.4f, 0.7f));
	SignText->SetRelativeLocation(SignBase + FVector(4.f, 0.f, 175.f));
	SignText->SetRelativeRotation(FRotator::ZeroRotator);
	SignText->SetText(FText::FromString(FString::Printf(TEXT("OBRA %s\n%.0f x %.0f m"),
		*ContractId, X * 2.f / 100.f, Y * 2.f / 100.f)));
}
