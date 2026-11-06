#include "CWInteractionComponent.h"

#include "CWInteractable.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/HitResult.h"
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
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams Params(TEXT("CWInteractionTrace"), false, Owner);
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(
		Hit,
		ViewLocation,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		Params);

	AActor* NewFocus = nullptr;
	if (bHit && Hit.GetActor() && Hit.GetActor()->Implements<UCWInteractable>())
	{
		NewFocus = Hit.GetActor();
	}

	FocusedActor = NewFocus;
}

void UCWInteractionComponent::Interact()
{
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
