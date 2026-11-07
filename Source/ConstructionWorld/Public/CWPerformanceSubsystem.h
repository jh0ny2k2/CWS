#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWPerformanceSubsystem.generated.h"

class FAutoConsoleCommand;

/**
 * Metricas de rendimiento del proyecto (F0.06).
 *
 * Mantiene una ventana movil de tiempos de frame y expone FPS medio, tiempo de
 * frame medio, percentil 95 y peor frame. Registra un resumen periodico en
 * LogCWPerf y ofrece comandos de consola:
 *
 *   cw.Perf.Hud 0|1        Muestra/oculta el overlay de metricas en el HUD.
 *   cw.Perf.Report 0|1     Activa/desactiva el resumen periodico en el log.
 *   cw.Perf.Interval 30    Segundos entre resumenes automaticos.
 *   cw.Perf.ReportNow      Escribe un resumen inmediato en el log.
 *   cw.Perf.Reset          Vacia la ventana de muestras.
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWPerformanceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem

	//~ Begin FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject

	/** FPS medio de la ventana actual (0 si aun no hay muestras). */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetAverageFPS() const { return AverageFPS; }

	/** Tiempo de frame medio en milisegundos. */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetAverageFrameMs() const { return AverageFrameMs; }

	/** Percentil 95 del tiempo de frame (ms): el "1% low" aproximado. */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetP95FrameMs() const { return P95FrameMs; }

	/** Peor tiempo de frame de la ventana (ms). */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetWorstFrameMs() const { return WorstFrameMs; }

	/** Numero de muestras validas en la ventana. */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	int32 GetSampleCount() const { return FrameTimesMs.Num(); }

	/** Escribe un resumen de metricas en LogCWPerf. */
	UFUNCTION(BlueprintCallable, Category = "CW|Performance")
	void LogReport();

	/** Vacia la ventana de muestras. */
	UFUNCTION(BlueprintCallable, Category = "CW|Performance")
	void ResetSamples();

	/** Indica si el overlay del HUD esta activo (cw.Perf.Hud). */
	static bool IsHudEnabled();

	/** Tamano maximo de la ventana movil de frames. */
	static constexpr int32 MaxSamples = 240;

private:
	/** Anade una muestra y recalcula las estadisticas. */
	void AddSample(float FrameMs);

	/** Recalcula media, percentil 95 y peor frame. */
	void RecalculateStats();

	TArray<float> FrameTimesMs;

	float AverageFPS = 0.f;
	float AverageFrameMs = 0.f;
	float P95FrameMs = 0.f;
	float WorstFrameMs = 0.f;
	float TimeSinceReport = 0.f;

	TUniquePtr<FAutoConsoleCommand> ReportNowCommand;
	TUniquePtr<FAutoConsoleCommand> ResetCommand;
};
