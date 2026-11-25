#include "CWHUD.h"

#include "CWContractSubsystem.h"
#include "CWDumpTruck.h"
#include "CWNotificationSubsystem.h"
#include "CWPlayerController.h"
#include "CWWorkParcel.h"
#include "CWExcavator.h"
#include "CWInteractionComponent.h"
#include "CWPerformanceSubsystem.h"
#include "CWPlayerCharacter.h"
#include "CWTerrainSubsystem.h"
#include "CWUnits.h"
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

	// F0.06: overlay de rendimiento (se oculta con cw.Perf.Hud 0).
	if (UCWPerformanceSubsystem::IsHudEnabled())
	{
		if (const UWorld* World = GetWorld())
		{
			if (const UCWPerformanceSubsystem* Perf = World->GetSubsystem<UCWPerformanceSubsystem>())
			{
				if (Perf->GetSampleCount() > 0)
				{
					const FString PerfText = FString::Printf(
						TEXT("%.0f FPS  |  %.2f ms  |  p95 %.2f ms"),
						Perf->GetAverageFPS(),
						Perf->GetAverageFrameMs(),
						Perf->GetP95FrameMs());

					DrawText(PerfText, FLinearColor(0.65f, 1.f, 0.65f, 0.9f), 16.f, 16.f, nullptr, 1.f, false);
				}
			}
		}
	}

	// H3: contrato, objetivo y avisos (se ocultan detras de los menus).
	const ACWPlayerController* CWPC = Cast<ACWPlayerController>(GetOwningPlayerController());
	if (!CWPC || !CWPC->IsMenuOpen())
	{
		if (const UWorld* World = GetWorld())
		{
			if (const UCWContractSubsystem* Contract = World->GetSubsystem<UCWContractSubsystem>())
			{
				DrawMissionPanel(*Contract);
				DrawObjectiveMarker(*Contract);
			}
		}
		DrawNotices();
	}

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

		// H1-B: paneles especificos de excavadora y volquete.
		if (const ACWExcavator* Excavator = Cast<ACWExcavator>(Pawn))
		{
			const FString Mode = Excavator->GetControlMode() == ECWExcavatorControlMode::Drive
				? TEXT("ORUGAS") : TEXT("EXCAVACION");
			const TCHAR* CameraName = Excavator->GetActiveCameraIndex() == 0 ? TEXT("Exterior")
				: (Excavator->GetActiveCameraIndex() == 1 ? TEXT("Cabina") : TEXT("Cuchara"));

			const FString Line1 = FString::Printf(
				TEXT("EXCAVADORA | %s | Motor: %s | Combustible: %.0f%% (%.0f L)"),
				*Mode,
				Excavator->IsEngineRunning() ? TEXT("ON") : TEXT("OFF"),
				Excavator->GetFuelRatio() * 100.f,
				Excavator->GetFuelLiters());

			const FString Line2 = FString::Printf(
				TEXT("Cuchara: %.2f / %.1f m3 de %s | Camara: %s | R modo | C camara | T excavar/vaciar | G motor | F salir"),
				Excavator->GetBucketLoadCubicMeters(),
				Excavator->GetBucketCapacityCubicMeters(),
				*Excavator->GetBucketSoilName(),
				CameraName);

			DrawText(Line1, FLinearColor(1.f, 0.85f, 0.f, 0.95f), 16.f, 40.f, nullptr, 0.9f, false);
			DrawText(Line2, FLinearColor(1.f, 0.85f, 0.f, 0.85f), 16.f, 60.f, nullptr, 0.9f, false);

			if (const UWorld* World = GetWorld())
			{
				if (const UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
				{
					if (Terrain->GetFieldCount() > 0)
					{
						const FString Line3 = FString::Printf(
							TEXT("Terreno: %.1f m3 | %.0f kg | %d campo(s)"),
							Terrain->GetTotalVolumeM3(),
							Terrain->GetTotalMassKg(),
							Terrain->GetFieldCount());
						DrawText(Line3, FLinearColor(0.75f, 0.95f, 0.75f, 0.85f), 16.f, 80.f, nullptr, 0.85f, false);
					}
				}
			}

			if (Excavator->GetActiveCameraIndex() == 1)
			{
				DrawCabinInstruments(*Excavator, CenterX, CenterY);
			}
		}
		else if (const ACWDumpTruck* Truck = Cast<ACWDumpTruck>(Pawn))
		{
			const FString Line1 = FString::Printf(
				TEXT("VOLQUETE | %.1f km/h | Carga: %.1f / %.1f m3 (%.0f%%, %.0f kg)"),
				CWUnits::CmsToKmh(Truck->GetSpeedCmS()),
				Truck->GetLoadCubicMeters(),
				Truck->GetCapacityCubicMeters(),
				Truck->GetFillRatio() * 100.f,
				Truck->GetLoadWeightKg());

			const FString Line2 = FString::Printf(
				TEXT("Caja: %.0f grados (%s) | %s | B caja | T descargar | F salir"),
				Truck->GetBedAngle(),
				Truck->IsBedRaisedForDump() ? TEXT("LISTA") : TEXT("BAJADA"),
				Truck->IsInsideDumpZone() ? TEXT("EN ZONA") : TEXT("FUERA DE ZONA"));

			DrawText(Line1, FLinearColor(1.f, 0.85f, 0.f, 0.95f), 16.f, 40.f, nullptr, 0.9f, false);
			DrawText(Line2, FLinearColor(1.f, 0.85f, 0.f, 0.85f), 16.f, 60.f, nullptr, 0.9f, false);
		}
		else
		{
			// Vehiculo base generico.
			const FString VehicleHint = TEXT("Conduciendo  |  WASD: mover  |  F: salir");
			float W = 0.f, H = 0.f;
			GetTextSize(VehicleHint, W, H, nullptr, 0.9f);
			DrawText(VehicleHint, FLinearColor(1.f, 0.85f, 0.f, 0.9f),
				CenterX - W * 0.5f, CenterY + 60.f, nullptr, 0.9f, false);
		}
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

void ACWHUD::DrawCabinInstruments(const ACWExcavator& Excavator, float CenterX, float CenterY)
{
	const float PanelWidth = 540.f;
	const float PanelHeight = 140.f;
	const float PanelX = CenterX - PanelWidth * 0.5f;
	const float PanelY = Canvas->SizeY - PanelHeight - 36.f;

	// Fondo y banda superior de la consola.
	DrawRect(FLinearColor(0.015f, 0.018f, 0.022f, 0.78f), PanelX, PanelY, PanelWidth, PanelHeight);
	DrawRect(FLinearColor(1.f, 0.72f, 0.05f, 0.95f), PanelX, PanelY, PanelWidth, 4.f);

	DrawText(TEXT("CABINA  ·  EXCAVADORA"), FLinearColor(1.f, 0.82f, 0.25f, 0.95f),
		PanelX + 16.f, PanelY + 10.f, nullptr, 0.85f, false);

	const FString EngineText = FString::Printf(TEXT("MOTOR  %.0f rpm"), Excavator.GetEngineRpm());
	DrawText(EngineText, FLinearColor(0.82f, 0.9f, 0.95f, 0.95f),
		PanelX + PanelWidth - 190.f, PanelY + 10.f, nullptr, 0.82f, false);

	auto DrawGauge = [this](float X, float Y, const FString& Label, const FString& Value,
		float Ratio, const FLinearColor& FillColor)
	{
		DrawText(Label, FLinearColor(0.72f, 0.78f, 0.82f, 0.95f), X, Y, nullptr, 0.78f, false);

		const float BarWidth = 220.f;
		const float BarHeight = 13.f;
		const float BarY = Y + 17.f;

		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), X, BarY, BarWidth, BarHeight);
		DrawRect(FillColor, X, BarY,
			BarWidth * FMath::Clamp(Ratio, 0.f, 1.f), BarHeight);

		DrawText(Value, FLinearColor::White, X + BarWidth + 8.f, Y + 2.f, nullptr, 0.75f, false);
	};

	const float LeftX = PanelX + 16.f;
	const float RightX = PanelX + 286.f;

	const float RpmRatio = FMath::Clamp(Excavator.GetEngineRpm() / 2300.f, 0.f, 1.f);
	const FLinearColor RpmColor = RpmRatio > 0.82f
		? FLinearColor(0.95f, 0.25f, 0.15f, 1.f)
		: FLinearColor(0.35f, 0.85f, 0.45f, 1.f);
	DrawGauge(LeftX, PanelY + 40.f, TEXT("REGIMEN"), FString::Printf(TEXT("%.0f rpm"),
		Excavator.GetEngineRpm()), RpmRatio, RpmColor);

	DrawGauge(RightX, PanelY + 40.f, TEXT("COMBUSTIBLE"),
		FString::Printf(TEXT("%.0f%%  %.0f L"), Excavator.GetFuelRatio() * 100.f,
			Excavator.GetFuelLiters()),
		Excavator.GetFuelRatio(), FLinearColor(1.f, 0.72f, 0.1f, 1.f));

	DrawGauge(LeftX, PanelY + 82.f, TEXT("PRESION HIDRAULICA"),
		FString::Printf(TEXT("%.0f bar"), Excavator.GetHydraulicPressureBar()),
		Excavator.GetHydraulicActivity(), FLinearColor(0.2f, 0.75f, 0.95f, 1.f));

	DrawGauge(RightX, PanelY + 82.f, TEXT("CUCHARA"),
		FString::Printf(TEXT("%.2f / %.1f m3"), Excavator.GetBucketLoadCubicMeters(),
			Excavator.GetBucketCapacityCubicMeters()),
		Excavator.GetBucketCapacityCubicMeters() > 0.f
			? Excavator.GetBucketLoadCubicMeters() / Excavator.GetBucketCapacityCubicMeters()
			: 0.f,
		FLinearColor(0.72f, 0.5f, 0.22f, 1.f));

	DrawText(TEXT("C: camara   Esc: pausa"), FLinearColor(0.55f, 0.6f, 0.65f, 0.8f),
		PanelX + 16.f, PanelY + 120.f, nullptr, 0.65f, false);
}

// --- H3: mision, objetivo y avisos ------------------------------------------------------

void ACWHUD::DrawBar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, W, H);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Ratio, 0.f, 1.f), H);
}

void ACWHUD::DrawMissionPanel(const UCWContractSubsystem& Contract)
{
	const FCWContractDefinition& Def = Contract.GetDefinition();
	const FCWContractProgress& P = Contract.GetProgress();

	const float W = 400.f;
	const float X = Canvas->SizeX - W - 18.f;
	float Y = 18.f;
	const FLinearColor Amber(1.f, 0.72f, 0.1f, 1.f);
	const FLinearColor Muted(0.72f, 0.75f, 0.8f, 0.95f);

	const bool bActive = Contract.IsActive();
	const float H = bActive ? 188.f : 84.f;
	DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.72f), X, Y, W, H);
	DrawRect(Amber, X, Y, 4.f, H);

	DrawText(FString::Printf(TEXT("%s  %s"), *Def.Id, *Def.Title), FLinearColor::White, X + 16.f, Y + 10.f, nullptr, 1.f, false);
	DrawText(FString::Printf(TEXT("%s  |  Saldo %s"), CWContract::StateName(P.State), *CWContract::FormatMoney(Contract.GetBalance())),
		Muted, X + 16.f, Y + 30.f, nullptr, 0.8f, false);

	if (!bActive)
	{
		DrawText(Contract.GetObjectiveText(), Amber, X + 16.f, Y + 54.f, nullptr, 0.82f, false);
		return;
	}

	// Tiempo restante (o transcurrido si no hay limite).
	FString TimeText = CWContract::FormatDuration(P.ElapsedSeconds);
	FLinearColor TimeColor = Muted;
	if (Def.TimeLimitSeconds > 0.f)
	{
		const float Left = FMath::Max(0.f, Def.TimeLimitSeconds - P.ElapsedSeconds);
		TimeText = FString::Printf(TEXT("Quedan %s"), *CWContract::FormatDuration(Left));
		TimeColor = Left < 300.f ? FLinearColor(1.f, 0.45f, 0.35f, 1.f) : Muted;
	}
	DrawText(TimeText, TimeColor, X + W - 120.f, Y + 10.f, nullptr, 0.85f, false);

	Y += 54.f;
	const float BarX = X + 16.f;
	const float BarW = W - 32.f;

	DrawText(FString::Printf(TEXT("Excavado en parcela  %.1f m3"), P.ExcavatedM3), Muted, BarX, Y, nullptr, 0.8f, false);
	DrawBar(BarX, Y + 17.f, BarW, 6.f, P.ExcavatedM3 / FMath::Max(0.1f, Def.TargetVolumeM3), FLinearColor(0.72f, 0.5f, 0.22f, 1.f));
	Y += 30.f;

	if (const ACWDumpTruck* Truck = Contract.FindTruck())
	{
		DrawText(FString::Printf(TEXT("Carga del volquete  %.1f / %.0f m3"), Truck->GetLoadCubicMeters(), Truck->GetCapacityCubicMeters()),
			Muted, BarX, Y, nullptr, 0.8f, false);
		DrawBar(BarX, Y + 17.f, BarW, 6.f, Truck->GetFillRatio(), FLinearColor(0.95f, 0.75f, 0.2f, 1.f));
		Y += 30.f;
	}

	DrawText(FString::Printf(TEXT("Entregado  %.1f / %.0f m3  (%.0f%%)"), P.DeliveredM3, Def.TargetVolumeM3, Contract.GetDeliveredRatio() * 100.f),
		FLinearColor::White, BarX, Y, nullptr, 0.85f, false);
	DrawBar(BarX, Y + 18.f, BarW, 8.f, Contract.GetDeliveredRatio(), FLinearColor(0.4f, 0.85f, 0.45f, 1.f));
	Y += 34.f;

	if (!Contract.AreHintsEnabled())
	{
		return;
	}

	// Objetivo actual con ajuste de linea manual (el canvas no envuelve el texto).
	TArray<FString> Words;
	Contract.GetObjectiveText().ParseIntoArray(Words, TEXT(" "));
	FString Line;
	int32 Lines = 0;
	for (const FString& Word : Words)
	{
		const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		float TW = 0.f, TH = 0.f;
		GetTextSize(Candidate, TW, TH, nullptr, 0.82f);
		if (TW > BarW && !Line.IsEmpty())
		{
			DrawText(Line, Amber, BarX, Y + Lines * 16.f, nullptr, 0.82f, false);
			++Lines;
			Line = Word;
		}
		else
		{
			Line = Candidate;
		}
	}
	if (!Line.IsEmpty())
	{
		DrawText(Line, Amber, BarX, Y + Lines * 16.f, nullptr, 0.82f, false);
	}
}

void ACWHUD::DrawObjectiveMarker(const UCWContractSubsystem& Contract)
{
	if (!Contract.AreHintsEnabled())
	{
		return;
	}

	FVector Target;
	FString Label;
	if (!Contract.GetObjectiveLocation(Target, Label))
	{
		return;
	}

	const APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const float DistanceM = Pawn ? FVector::Dist(Pawn->GetActorLocation(), Target) / 100.f : 0.f;
	if (DistanceM < 4.f)
	{
		return;
	}

	const FLinearColor Color(1.f, 0.72f, 0.1f, 0.95f);
	const FVector Screen = Project(Target, false);
	const float SizeX = Canvas->SizeX;
	const float SizeY = Canvas->SizeY;
	const float Margin = 60.f;
	const bool bOnScreen = Screen.Z > 0.f && Screen.X > Margin && Screen.X < SizeX - Margin
		&& Screen.Y > Margin && Screen.Y < SizeY - Margin;

	const FString Text = FString::Printf(TEXT("%s  %.0f m"), *Label, DistanceM);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, nullptr, 0.85f);

	if (bOnScreen)
	{
		// Rombo sobre el objetivo.
		const float S = 9.f;
		DrawLine(Screen.X, Screen.Y - S, Screen.X + S, Screen.Y, Color, 2.f);
		DrawLine(Screen.X + S, Screen.Y, Screen.X, Screen.Y + S, Color, 2.f);
		DrawLine(Screen.X, Screen.Y + S, Screen.X - S, Screen.Y, Color, 2.f);
		DrawLine(Screen.X - S, Screen.Y, Screen.X, Screen.Y - S, Color, 2.f);
		DrawText(Text, Color, Screen.X - TW * 0.5f, Screen.Y - S - TH - 4.f, nullptr, 0.85f, false);
		return;
	}

	// Fuera de pantalla: flecha en el borde apuntando hacia el objetivo.
	const FVector2D Center(SizeX * 0.5f, SizeY * 0.5f);
	FVector2D Dir(Screen.X - Center.X, Screen.Y - Center.Y);
	if (Screen.Z <= 0.f)
	{
		Dir = -Dir; // detras de la camara la proyeccion sale invertida
	}
	if (Dir.IsNearlyZero())
	{
		Dir = FVector2D(0.f, 1.f);
	}
	Dir.Normalize();

	const float HalfW = SizeX * 0.5f - Margin;
	const float HalfH = SizeY * 0.5f - Margin;
	const float Scale = FMath::Min(HalfW / FMath::Max(FMath::Abs(Dir.X), 0.001f), HalfH / FMath::Max(FMath::Abs(Dir.Y), 0.001f));
	const FVector2D Tip = Center + Dir * Scale;
	const FVector2D Perp(-Dir.Y, Dir.X);
	const FVector2D Base = Tip - Dir * 22.f;
	const FVector2D A = Base + Perp * 11.f;
	const FVector2D B = Base - Perp * 11.f;
	DrawLine(Tip.X, Tip.Y, A.X, A.Y, Color, 3.f);
	DrawLine(Tip.X, Tip.Y, B.X, B.Y, Color, 3.f);
	DrawLine(A.X, A.Y, B.X, B.Y, Color, 3.f);

	const FVector2D TextPos = Base - Dir * (18.f + FMath::Abs(Dir.X) * TW * 0.5f + FMath::Abs(Dir.Y) * TH);
	DrawText(Text, Color, FMath::Clamp(TextPos.X - TW * 0.5f, 8.f, SizeX - TW - 8.f),
		FMath::Clamp(TextPos.Y - TH * 0.5f, 8.f, SizeY - TH - 8.f), nullptr, 0.85f, false);
}

void ACWHUD::DrawNotices()
{
	const UWorld* World = GetWorld();
	const UCWNotificationSubsystem* Notices = World ? World->GetSubsystem<UCWNotificationSubsystem>() : nullptr;
	if (!Notices)
	{
		return;
	}

	const double Lifetime = 6.0;
	const TArray<FCWNotice> Active = Notices->GetActiveNotices(Lifetime);
	const double Now = Notices->GetNow();
	float Y = Canvas->SizeY * 0.70f;

	for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
	{
		const FCWNotice& Notice = Active[Index];
		const float Age = static_cast<float>(Now - Notice.TimeSeconds);
		const float Alpha = FMath::Clamp((static_cast<float>(Lifetime) - Age) / 1.2f, 0.f, 1.f);

		FLinearColor Accent(0.6f, 0.75f, 0.95f, 1.f);
		switch (Notice.Severity)
		{
		case ECWNoticeSeverity::Success: Accent = FLinearColor(0.4f, 0.9f, 0.45f, 1.f); break;
		case ECWNoticeSeverity::Warning: Accent = FLinearColor(1.f, 0.72f, 0.1f, 1.f); break;
		case ECWNoticeSeverity::Error:   Accent = FLinearColor(1.f, 0.4f, 0.35f, 1.f); break;
		default: break;
		}

		const FString Text = Notice.Count > 1 ? FString::Printf(TEXT("%s  (x%d)"), *Notice.Text, Notice.Count) : Notice.Text;
		float TW = 0.f, TH = 0.f;
		GetTextSize(Text, TW, TH, nullptr, 0.95f);
		const float BoxW = TW + 36.f;
		const float BoxX = (Canvas->SizeX - BoxW) * 0.5f;

		DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.75f * Alpha), BoxX, Y, BoxW, TH + 14.f);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, Alpha), BoxX, Y, 4.f, TH + 14.f);
		DrawText(Text, FLinearColor(1.f, 1.f, 1.f, Alpha), BoxX + 20.f, Y + 7.f, nullptr, 0.95f, false);
		Y -= TH + 20.f;
	}
}
