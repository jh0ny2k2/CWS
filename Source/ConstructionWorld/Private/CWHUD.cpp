#include "CWHUD.h"

#include "CWDumpTruck.h"
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
