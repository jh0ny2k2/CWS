#include "CWHUD.h"

#include "CWInteractionComponent.h"
#include "CWPlayerCharacter.h"
#include "CWVehicleBase.h"
#include "Engine/Canvas.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void ACWHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float CenterX = Canvas->SizeX * 0.5f;
	const float CenterY = Canvas->SizeY * 0.5f;

	// Punto de mira
	const FLinearColor CrosshairColor(1.f, 1.f, 1.f, 0.6f);
	DrawLine(CenterX - 8.f, CenterY, CenterX + 8.f, CenterY, CrosshairColor, 1.f);
	DrawLine(CenterX, CenterY - 8.f, CenterX, CenterY + 8.f, CrosshairColor, 1.f);

	APlayerController* PC = GetOwningPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;

	// BUG-008 corregido: si el pawn actual es un vehiculo, tomamos el InteractionComponent
	// del personaje que lo conduce, no del vehiculo (que no tiene ese componente).
	UCWInteractionComponent* Interaction = nullptr;
	if (const ACWVehicleBase* Vehicle = Cast<ACWVehicleBase>(Pawn))
	{
		if (ACWPlayerCharacter* Driver = Vehicle->GetDriver())
		{
			Interaction = Driver->FindComponentByClass<UCWInteractionComponent>();
		}

		// En el HUD del vehiculo mostramos el modo de control
		const FString VehicleHint = TEXT("Conduciendo  |  WASD: mover  |  F: salir");
		float W = 0.f, H = 0.f;
		GetTextSize(VehicleHint, W, H, nullptr, 0.9f);
		DrawText(VehicleHint, FLinearColor(1.f, 0.85f, 0.f, 0.9f),
			CenterX - W * 0.5f, CenterY + 60.f, nullptr, 0.9f, false);
	}
	else
	{
		Interaction = Pawn ? Pawn->FindComponentByClass<UCWInteractionComponent>() : nullptr;
	}

	if (!Interaction)
	{
		return;
	}

	const FText Prompt = Interaction->GetFocusPrompt();
	if (Prompt.IsEmpty())
	{
		return;
	}

	const FString PromptString = Prompt.ToString();
	float TextWidth = 0.f;
	float TextHeight = 0.f;
	GetTextSize(PromptString, TextWidth, TextHeight, nullptr, 1.1f);

	DrawText(
		PromptString,
		FLinearColor::White,
		CenterX - TextWidth * 0.5f,
		CenterY + 40.f,
		nullptr,
		1.1f,
		false);
}
