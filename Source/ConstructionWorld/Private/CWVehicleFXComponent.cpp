#include "CWVehicleFXComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* const DustSpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* const TrackPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* const FXMaterialPath =
		TEXT("/Game/ConstructionWorld/Materials/M_CW_TranslucentFX.M_CW_TranslucentFX");
}

UCWVehicleFXComponent::UCWVehicleFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
	SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(DustSpherePath);
	if (SphereFinder.Succeeded())
	{
		SphereMesh = SphereFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TrackPlanePath);
	if (PlaneFinder.Succeeded())
	{
		PlaneMesh = PlaneFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(FXMaterialPath);
	if (MaterialFinder.Succeeded())
	{
		FXMaterial = MaterialFinder.Object;
	}
}

void UCWVehicleFXComponent::BeginPlay()
{
	Super::BeginPlay();
	CreatePools();
}

void UCWVehicleFXComponent::CreatePools()
{
	if (bPoolsCreated)
	{
		return;
	}
	bPoolsCreated = true;

	UWorld* World = GetWorld();
	if (!World || !FXMaterial)
	{
		return;
	}

	// Polvo: esferas translucidas sin colision, creadas desactivadas.
	DustMeshes.Reserve(MaxDustPuffs);
	DustMIDs.Reserve(MaxDustPuffs);
	DustStates.SetNum(MaxDustPuffs);

	for (int32 Index = 0; Index < MaxDustPuffs; ++Index)
	{
		UStaticMeshComponent* Puff = NewObject<UStaticMeshComponent>(this,
			*FString::Printf(TEXT("CWDustPuff_%d"), Index));
		Puff->SetMobility(EComponentMobility::Movable);
		Puff->SetStaticMesh(SphereMesh);
		Puff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Puff->SetGenerateOverlapEvents(false);
		Puff->SetCastShadow(false);
		Puff->SetVisibility(false);
		Puff->RegisterComponent();

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FXMaterial, this);
		if (MID)
		{
			Puff->SetMaterial(0, MID);
		}

		DustMeshes.Add(Puff);
		DustMIDs.Add(MID);
	}

	// Huellas: planos oscuros en el mundo, tambien reutilizables.
	TrackMarkMID = UMaterialInstanceDynamic::Create(FXMaterial, this);
	if (TrackMarkMID)
	{
		TrackMarkMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.045f, 0.035f, 0.025f, 1.f));
		TrackMarkMID->SetScalarParameterValue(TEXT("Opacity"), 0.55f);
	}

	TrackMarkMeshes.Reserve(MaxTrackMarks);
	TrackMarkAges.Reserve(MaxTrackMarks);
	for (int32 Index = 0; Index < MaxTrackMarks; ++Index)
	{
		UStaticMeshComponent* Mark = NewObject<UStaticMeshComponent>(this,
			*FString::Printf(TEXT("CWTrackMark_%d"), Index));
		Mark->SetMobility(EComponentMobility::Movable);
		Mark->SetStaticMesh(PlaneMesh);
		Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mark->SetGenerateOverlapEvents(false);
		Mark->SetCastShadow(false);
		Mark->SetVisibility(false);
		Mark->RegisterComponent();

		if (TrackMarkMID)
		{
			Mark->SetMaterial(0, TrackMarkMID);
		}

		TrackMarkMeshes.Add(Mark);
		TrackMarkAges.Add(0.f);
	}

	if (AActor* Owner = GetOwner())
	{
		LastTrackMarkLocation = Owner->GetActorLocation();
		bHasLastTrackMarkLocation = true;
	}
}

void UCWVehicleFXComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPoolsCreated)
	{
		return;
	}

	UpdateDust(DeltaTime);

	if (bTrackMarksEnabled)
	{
		if (const AActor* Owner = GetOwner())
		{
			UpdateTrackMarks(DeltaTime, Owner->GetActorLocation());
		}
	}
	else
	{
		UpdateTrackMarks(DeltaTime, FVector::ZeroVector);
	}
}

void UCWVehicleFXComponent::EmitDust(const FVector& WorldLocation, const FLinearColor& Color,
	int32 PuffCount, float Strength)
{
	if (!bPoolsCreated || DustMeshes.Num() == 0)
	{
		return;
	}

	const int32 Count = FMath::Clamp(PuffCount, 1, MaxDustPuffs);
	const float SafeStrength = FMath::Clamp(Strength, 0.2f, 3.f);

	int32 Emitted = 0;
	for (int32 Index = 0; Index < DustStates.Num() && Emitted < Count; ++Index)
	{
		FCWDustPuffState& State = DustStates[Index];
		if (State.bActive)
		{
			continue;
		}

		UStaticMeshComponent* Puff = DustMeshes[Index];
		UMaterialInstanceDynamic* MID = DustMIDs[Index];
		if (!Puff || !MID)
		{
			continue;
		}

		State.bActive = true;
		State.Age = 0.f;
		State.Life = FMath::FRandRange(1.1f, 2.2f);
		State.StartScale = FMath::FRandRange(0.12f, 0.30f) * SafeStrength;
		State.EndScale = FMath::FRandRange(0.9f, 1.8f) * SafeStrength;
		State.Velocity = FVector(
			FMath::FRandRange(-70.f, 70.f),
			FMath::FRandRange(-70.f, 70.f),
			FMath::FRandRange(25.f, 90.f)) * SafeStrength;

		const FVector Offset(
			FMath::FRandRange(-35.f, 35.f),
			FMath::FRandRange(-35.f, 35.f),
			FMath::FRandRange(-10.f, 25.f));

		Puff->SetWorldLocation(WorldLocation + Offset);
		Puff->SetWorldScale3D(FVector(State.StartScale));
		Puff->SetVisibility(true);

		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), 0.42f);

		++Emitted;
	}
}

void UCWVehicleFXComponent::UpdateDust(float DeltaTime)
{
	for (int32 Index = 0; Index < DustStates.Num(); ++Index)
	{
		FCWDustPuffState& State = DustStates[Index];
		if (!State.bActive)
		{
			continue;
		}

		State.Age += DeltaTime;
		UStaticMeshComponent* Puff = DustMeshes[Index];
		UMaterialInstanceDynamic* MID = DustMIDs[Index];
		if (!Puff || !MID)
		{
			State.bActive = false;
			continue;
		}

		if (State.Age >= State.Life)
		{
			State.bActive = false;
			Puff->SetVisibility(false);
			continue;
		}

		const float Alpha = State.Age / State.Life;
		State.Velocity *= FMath::Clamp(1.f - DeltaTime * 1.8f, 0.f, 1.f);

		const FVector NewLocation = Puff->GetComponentLocation() + State.Velocity * DeltaTime;
		Puff->SetWorldLocation(NewLocation);

		const float Scale = FMath::Lerp(State.StartScale, State.EndScale, Alpha);
		Puff->SetWorldScale3D(FVector(Scale));

		const float Opacity = 0.42f * FMath::Pow(1.f - Alpha, 1.35f);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}

void UCWVehicleFXComponent::UpdateTrackMarks(float DeltaTime, const FVector& OwnerLocation)
{
	// Envejecer y ocultar huellas caducadas.
	for (int32 Index = 0; Index < TrackMarkMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Mark = TrackMarkMeshes[Index];
		if (!Mark || !Mark->IsVisible())
		{
			continue;
		}

		TrackMarkAges[Index] += DeltaTime;
		if (TrackMarkAges[Index] >= TrackMarkLifeSeconds)
		{
			Mark->SetVisibility(false);
		}
	}

	const AActor* Owner = GetOwner();
	if (!bTrackMarksEnabled || !Owner || !bHasLastTrackMarkLocation)
	{
		return;
	}

	const FVector Delta = OwnerLocation - LastTrackMarkLocation;
	if (Delta.SizeSquared2D() < FMath::Square(TrackMarkSpacingCm))
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	Owner->GetActorBounds(true, Origin, Extent);

	const float HalfGauge = FMath::Clamp(Extent.Y * 0.55f, 25.f, 170.f);
	const FVector Right = Owner->GetActorRotation().RotateVector(FVector::RightVector);
	const FRotator YawRotation(0.f, Owner->GetActorRotation().Yaw, 0.f);

	for (int32 Side = 0; Side < 2; ++Side)
	{
		int32 Slot = INDEX_NONE;
		for (int32 Index = 0; Index < TrackMarkMeshes.Num(); ++Index)
		{
			if (TrackMarkMeshes[Index] && !TrackMarkMeshes[Index]->IsVisible())
			{
				Slot = Index;
				break;
			}
		}

		if (Slot == INDEX_NONE)
		{
			break;
		}

		const float Sign = (Side == 0) ? -1.f : 1.f;
		const FVector MarkLocation = OwnerLocation + Right * (HalfGauge * Sign);
		PlaceTrackMark(Slot, MarkLocation, YawRotation, 22.f, 46.f);
	}

	LastTrackMarkLocation = OwnerLocation;
}

bool UCWVehicleFXComponent::FindGroundZ(const FVector& WorldLocation, float& OutGroundZ) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(TEXT("CWTrackGround"), false, GetOwner());
	FHitResult Hit;
	const FVector Start = WorldLocation + FVector(0.f, 0.f, 250.f);
	const FVector End = WorldLocation - FVector(0.f, 0.f, 700.f);

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		OutGroundZ = Hit.ImpactPoint.Z;
		return true;
	}

	return false;
}

void UCWVehicleFXComponent::PlaceTrackMark(int32 Index, const FVector& WorldLocation,
	const FRotator& WorldRotation, float HalfWidthCm, float HalfLengthCm)
{
	if (!TrackMarkMeshes.IsValidIndex(Index) || !TrackMarkMeshes[Index])
	{
		return;
	}

	float GroundZ = WorldLocation.Z;
	FindGroundZ(WorldLocation, GroundZ);

	UStaticMeshComponent* Mark = TrackMarkMeshes[Index];
	Mark->SetWorldLocation(FVector(WorldLocation.X, WorldLocation.Y, GroundZ + 1.5f));
	Mark->SetWorldRotation(WorldRotation);
	Mark->SetWorldScale3D(FVector(HalfWidthCm * 2.f / 100.f, HalfLengthCm * 2.f / 100.f, 1.f));
	Mark->SetVisibility(true);

	TrackMarkAges[Index] = 0.f;
}
