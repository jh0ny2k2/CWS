#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CWPlayerController.h"

class SBox;

/**
 * Menus del juego en Slate (F4.07): principal, informe del contrato, pausa, ajustes y
 * liquidacion. Toda la logica vive en ACWPlayerController; este widget solo dibuja.
 */
class CONSTRUCTIONWORLD_API SCWMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCWMenuWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ACWPlayerController>, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ShowPage(ECWMenuPage Page);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TSharedRef<SWidget> BuildMain();
	TSharedRef<SWidget> BuildBriefing();
	TSharedRef<SWidget> BuildPause();
	TSharedRef<SWidget> BuildSettings();
	TSharedRef<SWidget> BuildSummary();

	TSharedRef<SWidget> MakePanel(const FString& Title, const FString& Subtitle, TSharedRef<SWidget> Body);
	TSharedRef<SWidget> MakeButton(const FString& Label, TFunction<void()> OnClick, bool bEnabled = true, bool bPrimary = false);
	TSharedRef<SWidget> MakeLine(const FString& Label, const FString& Value, FLinearColor ValueColor = FLinearColor::White);

	TWeakObjectPtr<ACWPlayerController> Owner;
	TSharedPtr<SBox> Content;
};
