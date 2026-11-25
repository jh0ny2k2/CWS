#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWPerformanceSubsystem.generated.h"

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

protected:
	/** Solo mundos de juego (Game/PIE): no medir el viewport del editor ni previews. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:

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

	/** Percentil 95 del tiempo de frame (ms): solo el 5% de frames es mas lento. */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetP95FrameMs() const;

	/** Peor tiempo de frame de la ventana (ms). */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	float GetWorstFrameMs() const { return WorstFrameMs; }

	/** Numero de muestras validas en la ventana. */
	UFUNCTION(BlueprintPure, Category = "CW|Performance")
	int32 GetSampleCount() const { return SampleCount; }

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
	/** Anade una muestra al buffer circular y actualiza media y peor frame (sin copias). */
	void AddSample(float FrameMs);

	/** Buffer circular de tiempos de frame (ms). */
	TArray<float> FrameTimesMs;
	int32 NextIndex = 0;
	int32 SampleCount = 0;
	double SumMs = 0.0;

	float AverageFPS = 0.f;
	float AverageFrameMs = 0.f;
	float WorstFrameMs = 0.f;
	float TimeSinceReport = 0.f;

	/** El p95 exige ordenar: se calcula bajo demanda y se cachea hasta la siguiente muestra. */
	mutable float CachedP95FrameMs = 0.f;
	mutable bool bP95Dirty = true;
};
