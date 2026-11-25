#include "CWInteractionComponent.h"

#include "CWInteractable.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UCWInteractionComponent::UCWInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UCWInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFocus();
}

void UCWInteractionComponent::UpdateFocus()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		FocusedActor = nullptr;
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDir = ViewRotation.Vector();

	// 1) Candidatos: todo lo que bloquea/solapa visibilidad dentro del radio de busqueda.
	FCollisionQueryParams Params(TEXT("CWInteractionQuery"), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps, ViewLocation, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceDistance), Params);

	const float MinCos = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MaxAimAngleDegrees, 1.f, 89.f)));

	AActor* Best = nullptr;
	int32 BestPriority = MIN_int32;
	float BestScore = -BIG_NUMBER;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		UPrimitiveComponent* Component = Overlap.GetComponent();
		// Un actor puede aparecer con varios componentes: se evalua cada uno y gana el mejor.
		if (!Candidate || !Component || !Candidate->Implements<UCWInteractable>())
		{
			continue;
		}

		if (!ICWInteractable::Execute_CanInteract(Candidate, Owner))
		{
			continue;
		}

		// 2) Distancia al punto mas cercano de la colision (no al origen del actor: una
		//    excavadora mide varios metros y su pivote puede quedar lejos de la puerta).
		FVector ClosestPoint = Component->GetComponentLocation();
		if (Component->GetClosestPointOnCollision(ViewLocation, ClosestPoint) < 0.f)
		{
			ClosestPoint = Component->GetComponentLocation();
		}

		const float ActorMax = ICWInteractable::Execute_GetMaxInteractionDistance(Candidate);
		const float MaxDistance = ActorMax > 0.f ? FMath::Min(ActorMax, TraceDistance) : TraceDistance;
		const FVector ToPoint = ClosestPoint - ViewLocation;
		const float Distance = ToPoint.Size();
		if (Distance > MaxDistance)
		{
			continue;
		}

		// 3) Debe estar en el cono de mira (salvo si estamos pegados a el).
		const float AimCos = Distance > TraceRadius ? FVector::DotProduct(ViewDir, ToPoint / Distance) : 1.f;
		if (AimCos < MinCos)
		{
			continue;
		}

		// 4) Linea de vision: nada de otro actor entre la vista y el candidato.
		FHitResult Hit;
		if (Distance > TraceRadius
			&& World->LineTraceSingleByChannel(Hit, ViewLocation, ClosestPoint, ECC_Visibility, Params)
			&& Hit.GetActor() != Candidate
			&& Hit.Distance < Distance - 5.f)
		{
			continue;
		}

		// 5) Prioridad declarada primero; a igualdad, lo mas centrado y cercano.
		const int32 Priority = ICWInteractable::Execute_GetInteractionPriority(Candidate);
		const float Score = AimCos - 0.25f * (Distance / FMath::Max(MaxDistance, 1.f));
		if (Priority > BestPriority || (Priority == BestPriority && Score > BestScore))
		{
			Best = Candidate;
			BestPriority = Priority;
			BestScore = Score;
		}
	}

	FocusedActor = Best;
}

void UCWInteractionComponent::Interact()
{
	// Revalidar en el momento de pulsar: el foco puede tener hasta un TickInterval de antiguedad.
	UpdateFocus();

	if (FocusedActor && FocusedActor->Implements<UCWInteractable>())
	{
		ICWInteractable::Execute_Interact(FocusedActor, GetOwner());
	}
}

FText UCWInteractionComponent::GetFocusPrompt() const
{
	if (FocusedActor && FocusedActor->Implements<UCWInteractable>())
	{
		return ICWInteractable::Execute_GetInteractionPrompt(FocusedActor);
	}

	return FText::GetEmpty();
}
