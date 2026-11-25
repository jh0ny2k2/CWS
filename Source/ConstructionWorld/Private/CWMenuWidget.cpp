#include "CWMenuWidget.h"

#include "CWContractSubsystem.h"
#include "Engine/World.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor Amber(1.f, 0.72f, 0.1f, 1.f);
	const FLinearColor Muted(0.72f, 0.74f, 0.78f, 1.f);
	const FLinearColor PanelColor(FColor(17, 19, 23, 242));
	const FLinearColor Good(0.45f, 0.9f, 0.5f, 1.f);
	const FLinearColor Bad(1.f, 0.45f, 0.4f, 1.f);

	const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush("WhiteBrush");
	}

	UCWContractSubsystem* GetContract(const TWeakObjectPtr<ACWPlayerController>& Owner)
	{
		const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UCWContractSubsystem>() : nullptr;
	}

	/** Coste en negativo; cero se muestra sin signo. */
	FString Cost(float Euros)
	{
		return Euros > 0.5f ? TEXT("-") + CWContract::FormatMoney(Euros) : CWContract::FormatMoney(0.f);
	}

	/** Botones planos con esquinas suaves (colores en sRGB: FLinearColor(FColor) convierte bien). */
	FButtonStyle MakeFlatStyle(FColor Normal, FColor Hovered, FColor Pressed)
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(Normal), 4.f));
		Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(Hovered), 4.f));
		Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(Pressed), 4.f));
		Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(FColor(30, 32, 37)), 4.f));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	const FButtonStyle& FlatButtonStyle(bool bPrimary)
	{
		static const FButtonStyle Primary = MakeFlatStyle(FColor(255, 186, 32), FColor(255, 207, 92), FColor(222, 152, 12));
		static const FButtonStyle Secondary = MakeFlatStyle(FColor(44, 48, 56), FColor(64, 70, 82), FColor(34, 37, 43));
		return bPrimary ? Primary : Secondary;
	}
}

void SCWMenuWidget::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(WhiteBrush())
			.ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(Content, SBox)
			.WidthOverride(560.f)
		]
	];
}

void SCWMenuWidget::ShowPage(ECWMenuPage Page)
{
	if (!Content.IsValid())
	{
		return;
	}

	switch (Page)
	{
	case ECWMenuPage::Main:     Content->SetContent(BuildMain()); break;
	case ECWMenuPage::Briefing: Content->SetContent(BuildBriefing()); break;
	case ECWMenuPage::Pause:    Content->SetContent(BuildPause()); break;
	case ECWMenuPage::Settings: Content->SetContent(BuildSettings()); break;
	case ECWMenuPage::Summary:  Content->SetContent(BuildSummary()); break;
	default:                    Content->SetContent(SNullWidget::NullWidget); break;
	}
}

FReply SCWMenuWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P
		|| InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (Owner.IsValid())
		{
			Owner->MenuBack();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// --- Piezas comunes -----------------------------------------------------------------

TSharedRef<SWidget> SCWMenuWidget::MakePanel(const FString& Title, const FString& Subtitle, TSharedRef<SWidget> Body)
{
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(PanelColor)
		.Padding(FMargin(36.f, 30.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(4.f).WidthOverride(64.f).HAlign(HAlign_Left)
				[
					SNew(SImage).Image(WhiteBrush()).ColorAndOpacity(Amber)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 2.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Title))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Subtitle))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.ColorAndOpacity(Muted)
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Body
			]
		];
}

TSharedRef<SWidget> SCWMenuWidget::MakeButton(const FString& Label, TFunction<void()> OnClick, bool bEnabled, bool bPrimary)
{
	return SNew(SBox)
		.HeightOverride(44.f)
		.Padding(FMargin(0.f, 3.f))
		[
			SNew(SButton)
			.IsEnabled(bEnabled)
			.ButtonStyle(&FlatButtonStyle(bPrimary))
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.ContentPadding(FMargin(16.f, 4.f))
			.OnClicked_Lambda([OnClick]()
			{
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				.ColorAndOpacity(!bEnabled ? FLinearColor(1.f, 1.f, 1.f, 0.3f) : (bPrimary ? FLinearColor(FColor(20, 18, 12)) : FLinearColor::White))
			]
		];
}

TSharedRef<SWidget> SCWMenuWidget::MakeLine(const FString& Label, const FString& Value, FLinearColor ValueColor)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 3.f)
		[
			SNew(STextBlock).Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Muted)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 3.f)
		[
			SNew(STextBlock).Text(FText::FromString(Value))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(ValueColor)
		];
}

// --- Paginas ------------------------------------------------------------------------

TSharedRef<SWidget> SCWMenuWidget::BuildMain()
{
	TWeakObjectPtr<ACWPlayerController> O = Owner;
	const bool bHasSave = UCWContractSubsystem::HasSave(UCWContractSubsystem::DefaultSlot);

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Continuar partida"), [O]() { if (O.IsValid()) { O->ContinueGame(); } }, bHasSave, bHasSave)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Nueva partida"), [O]() { if (O.IsValid()) { O->StartNewGame(); } }, true, !bHasSave)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Ajustes"), [O]() { if (O.IsValid()) { O->ShowMenu(ECWMenuPage::Settings); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Salir"), [O]() { if (O.IsValid()) { O->QuitGame(); } })];

	return MakePanel(TEXT("CONSTRUCTION WORLD"),
		TEXT("Simulador de obra. Primer contrato: vaciado de parcela (CWS-001)."), Body);
}

TSharedRef<SWidget> SCWMenuWidget::BuildBriefing()
{
	TWeakObjectPtr<ACWPlayerController> O = Owner;
	const UCWContractSubsystem* Contract = GetContract(Owner);
	const FCWContractDefinition Def = Contract ? Contract->GetDefinition() : FCWContractDefinition();

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
	[
		SNew(STextBlock).Text(FText::FromString(Def.Description))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).ColorAndOpacity(FLinearColor::White).AutoWrapText(true)
	];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Cliente"), Def.Client)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Objetivo"), FString::Printf(TEXT("Entregar %.0f m3 de tierra"), Def.TargetVolumeM3))];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Pago al completar"), CWContract::FormatMoney(Def.Reward), Good)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Plazo"), Def.TimeLimitSeconds > 0.f ? CWContract::FormatDuration(Def.TimeLimitSeconds) + TEXT(" min") : TEXT("Sin limite"))];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Costes"), FString::Printf(TEXT("Gasoil %.2f EUR/L  |  Maquinas %.0f EUR/h"), Def.FuelPricePerLiter, Def.MachineHourlyRate))];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Penalizaciones"), FString::Printf(TEXT("Fuera de parcela %.0f  |  Vertido ilegal %.0f EUR/m3"), Def.OutsideDigPenaltyPerM3, Def.IllegalDumpPenaltyPerM3), Bad)];
	Body->AddSlot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[MakeButton(TEXT("Aceptar contrato"), [O]() { if (O.IsValid()) { O->AcceptContractFromBriefing(); } }, true, true)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Volver"), [O]() { if (O.IsValid()) { O->MenuBack(); } })];

	return MakePanel(FString::Printf(TEXT("%s  %s"), *Def.Id, *Def.Title),
		TEXT("Te guiara un tutorial paso a paso. Puedes desactivar las pistas en Ajustes."), Body);
}

TSharedRef<SWidget> SCWMenuWidget::BuildPause()
{
	TWeakObjectPtr<ACWPlayerController> O = Owner;
	const UCWContractSubsystem* Contract = GetContract(Owner);
	const bool bHasSave = UCWContractSubsystem::HasSave(UCWContractSubsystem::DefaultSlot);

	FString Subtitle = TEXT("Juego en pausa.");
	if (Contract)
	{
		const FCWContractProgress& P = Contract->GetProgress();
		Subtitle = FString::Printf(TEXT("%s  |  %s  |  Entregado %.1f / %.0f m3  |  Tiempo %s  |  Saldo %s"),
			*Contract->GetDefinition().Id, CWContract::StateName(P.State), P.DeliveredM3,
			Contract->GetDefinition().TargetVolumeM3, *CWContract::FormatDuration(P.ElapsedSeconds),
			*CWContract::FormatMoney(Contract->GetBalance()));
	}

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Reanudar"), [O]() { if (O.IsValid()) { O->CloseMenu(); } }, true, true)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Guardar partida  (F5)"), [O]() { if (O.IsValid()) { O->QuickSave(); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Cargar partida  (F9)"), [O]() { if (O.IsValid()) { O->QuickLoad(); } }, bHasSave)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Reiniciar mision"), [O]() { if (O.IsValid()) { O->RestartMission(); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Ajustes"), [O]() { if (O.IsValid()) { O->ShowMenu(ECWMenuPage::Settings); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Menu principal"), [O]() { if (O.IsValid()) { O->ReturnToMainMenu(); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Salir del juego"), [O]() { if (O.IsValid()) { O->QuitGame(); } })];

	return MakePanel(TEXT("PAUSA"), Subtitle, Body);
}

TSharedRef<SWidget> SCWMenuWidget::BuildSettings()
{
	TWeakObjectPtr<ACWPlayerController> O = Owner;

	auto SliderRow = [](const FString& Label, TFunction<float()> Get, TFunction<void(float)> Set, float Min, float Max)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([Get]() { return FText::FromString(FString::Printf(TEXT("%.2f"), Get())); })
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SSlider)
				.MinValue(Min)
				.MaxValue(Max)
				.Value_Lambda([Get]() { return Get(); })
				.OnValueChanged_Lambda([Set](float Value) { Set(Value); })
				.SliderBarColor(FLinearColor(0.3f, 0.3f, 0.32f, 1.f))
				.SliderHandleColor(Amber)
			];
	};

	auto CheckRow = [](const FString& Label, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		return SNew(SBox).Padding(FMargin(0.f, 6.f))
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([Get]() { return Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([Set](ECheckBoxState State) { Set(State == ECheckBoxState::Checked); })
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(FLinearColor::White)
				]
			];
	};

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()[SliderRow(TEXT("Sensibilidad del raton"),
		[O]() { return O.IsValid() ? O->GetMouseSensitivity() : 1.f; },
		[O](float V) { if (O.IsValid()) { O->SetMouseSensitivity(V); } }, 0.2f, 3.f)];
	Body->AddSlot().AutoHeight()[SliderRow(TEXT("Volumen general"),
		[O]() { return O.IsValid() ? O->GetMasterVolume() : 1.f; },
		[O](float V) { if (O.IsValid()) { O->SetMasterVolume(V); } }, 0.f, 1.f)];
	Body->AddSlot().AutoHeight()[CheckRow(TEXT("Invertir eje vertical de la camara"),
		[O]() { return O.IsValid() && O->GetInvertY(); },
		[O](bool V) { if (O.IsValid()) { O->SetInvertY(V); } })];
	Body->AddSlot().AutoHeight()[CheckRow(TEXT("Mostrar pistas del tutorial y marcador de objetivo"),
		[O]() { return O.IsValid() && O->GetShowHints(); },
		[O](bool V) { if (O.IsValid()) { O->SetShowHints(V); } })];
	Body->AddSlot().AutoHeight()[CheckRow(TEXT("Mostrar FPS y tiempo de frame"),
		[O]() { return O.IsValid() && O->GetShowPerf(); },
		[O](bool V) { if (O.IsValid()) { O->SetShowPerf(V); } })];
	Body->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[MakeButton(TEXT("Volver"), [O]() { if (O.IsValid()) { O->MenuBack(); } }, true, true)];

	return MakePanel(TEXT("AJUSTES"), TEXT("Los cambios se aplican al momento y se guardan."), Body);
}

TSharedRef<SWidget> SCWMenuWidget::BuildSummary()
{
	TWeakObjectPtr<ACWPlayerController> O = Owner;
	const UCWContractSubsystem* Contract = GetContract(Owner);

	FCWContractResult R;
	FCWContractProgress P;
	FCWContractDefinition Def;
	float Balance = 0.f;
	if (Contract)
	{
		P = Contract->GetProgress();
		Def = Contract->GetDefinition();
		R = Contract->GetLastResult().bValid ? Contract->GetLastResult() : Contract->ComputeSettlement();
		Balance = Contract->GetBalance();
	}
	const bool bCompleted = P.State == ECWContractState::Completed;

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Material entregado"), FString::Printf(TEXT("%.1f / %.0f m3 en %d viaje(s)"), P.DeliveredM3, Def.TargetVolumeM3, P.Trips))];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Tiempo"), CWContract::FormatDuration(P.ElapsedSeconds))];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Pago del cliente"), CWContract::FormatMoney(R.Reward), Good)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Gasoil"), FString::Printf(TEXT("%s  (%.1f L)"), *Cost(R.FuelCost), P.FuelUsedLiters), R.FuelCost > 0.5f ? Bad : Muted)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Horas de maquina"), Cost(R.MachineCost), R.MachineCost > 0.5f ? Bad : Muted)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Penalizaciones"), FString::Printf(TEXT("%s  (%.1f m3 fuera, %.1f m3 vertidos)"),
		*Cost(R.Penalties), P.OutsideDigM3, P.IllegalDumpM3), R.Penalties > 0.f ? Bad : Muted)];
	Body->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
	[
		SNew(SBox).HeightOverride(1.f)[SNew(SImage).Image(WhiteBrush()).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.15f))]
	];
	Body->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[MakeLine(TEXT("Beneficio neto"), CWContract::FormatMoney(R.Net), R.Net >= 0.f ? Good : Bad)];
	Body->AddSlot().AutoHeight()[MakeLine(TEXT("Saldo de la empresa"), CWContract::FormatMoney(Balance))];
	Body->AddSlot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[MakeButton(TEXT("Repetir contrato"), [O]() { if (O.IsValid()) { O->RestartMission(); } }, true, true)];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Seguir en la obra"), [O]() { if (O.IsValid()) { O->CloseMenu(); } })];
	Body->AddSlot().AutoHeight()[MakeButton(TEXT("Menu principal"), [O]() { if (O.IsValid()) { O->ReturnToMainMenu(); } })];

	return MakePanel(bCompleted ? TEXT("CONTRATO COMPLETADO") : TEXT("CONTRATO NO COMPLETADO"),
		FString::Printf(TEXT("%s  %s  |  %s"), *Def.Id, *Def.Title, *Def.Client), Body);
}
