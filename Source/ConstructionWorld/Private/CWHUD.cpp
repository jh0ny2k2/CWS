#include "CWHUD.h"

#include "CWInteractionComponent.h"
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

	const FLinearColor CrosshairColor(1.f, 1.f, 1.f, 0.6f);
	DrawLine(CenterX - 8.f, CenterY, CenterX + 8.f, CenterY, CrosshairColor, 1.f);
	DrawLine(CenterX, CenterY - 8.f, CenterX, CenterY + 8.f, CrosshairColor, 1.f);

	APlayerController* PC = GetOwningPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UCWInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UCWInteractionComponent>() : nullptr;
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
