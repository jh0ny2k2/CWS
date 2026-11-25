#include "CWPerformanceSubsystem.h"

#include "CWLog.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Stats/Stats.h"

static TAutoConsoleVariable<int32> CVarCWPerfHud(
	TEXT("cw.Perf.Hud"),
	1,
	TEXT("Muestra el overlay de FPS y tiempo de frame en el HUD (1 = si, 0 = no)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarCWPerfReport(
	TEXT("cw.Perf.Report"),
	1,
	TEXT("Escribe un resumen de rendimiento periodico en LogCWPerf (1 = si, 0 = no)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarCWPerfInterval(
	TEXT("cw.Perf.Interval"),
	30.f,
	TEXT("Segundos entre resumenes automaticos de rendimiento."),
	ECVF_Default);

// Comandos registrados una sola vez por proceso; actuan sobre el subsistema del mundo
// desde el que se lanzan (antes se registraban por instancia y chocaban con varios mundos).
static FAutoConsoleCommandWithWorld GCWPerfReportNowCommand(
	TEXT("cw.Perf.ReportNow"),
	TEXT("Escribe un resumen inmediato de FPS/tiempo de frame en el log."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UCWPerformanceSubsystem* Perf = World ? World->GetSubsystem<UCWPerformanceSubsystem>() : nullptr)
		{
			Perf->LogReport();
		}
	}));

static FAutoConsoleCommandWithWorld GCWPerfResetCommand(
	TEXT("cw.Perf.Reset"),
	TEXT("Vacia la ventana de muestras de rendimiento."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UCWPerformanceSubsystem* Perf = World ? World->GetSubsystem<UCWPerformanceSubsystem>() : nullptr)
		{
			Perf->ResetSamples();
		}
	}));

bool UCWPerformanceSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCWPerformanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FrameTimesMs.SetNumZeroed(MaxSamples);
	ResetSamples();

	UE_LOG(LogCWPerf, Log,
		TEXT("Metricas de rendimiento activas (ventana=%d frames). Comandos: cw.Perf.Hud, cw.Perf.ReportNow, cw.Perf.Reset."),
		MaxSamples);
}

void UCWPerformanceSubsystem::Deinitialize()
{
	FrameTimesMs.Empty();
	SampleCount = 0;

	Super::Deinitialize();
}

void UCWPerformanceSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Ignorar pausas, frames invalidos y cargas de nivel.
	if (DeltaTime <= 0.f || DeltaTime > 2.f)
	{
		return;
	}

	AddSample(DeltaTime * 1000.f);

	if (CVarCWPerfReport.GetValueOnGameThread() == 0)
	{
		return;
	}

	const float Interval = FMath::Max(1.f, CVarCWPerfInterval.GetValueOnGameThread());
	TimeSinceReport += DeltaTime;
	if (TimeSinceReport >= Interval)
	{
		TimeSinceReport = 0.f;
		LogReport();
	}
}

TStatId UCWPerformanceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCWPerformanceSubsystem, STATGROUP_Tickables);
}

bool UCWPerformanceSubsystem::IsHudEnabled()
{
	return CVarCWPerfHud.GetValueOnGameThread() != 0;
}

void UCWPerformanceSubsystem::AddSample(float FrameMs)
{
	if (FrameTimesMs.Num() != MaxSamples)
	{
		FrameTimesMs.SetNumZeroed(MaxSamples);
	}

	const bool bFull = SampleCount >= MaxSamples;
	const float Evicted = bFull ? FrameTimesMs[NextIndex] : 0.f;

	FrameTimesMs[NextIndex] = FrameMs;
	NextIndex = (NextIndex + 1) % MaxSamples;
	SampleCount = FMath::Min(SampleCount + 1, MaxSamples);
	SumMs += FrameMs - Evicted;

	AverageFrameMs = static_cast<float>(SumMs / SampleCount);
	AverageFPS = AverageFrameMs > KINDA_SMALL_NUMBER ? (1000.f / AverageFrameMs) : 0.f;

	// El peor frame solo exige recorrer la ventana si el que sale era el maximo.
	if (FrameMs >= WorstFrameMs)
	{
		WorstFrameMs = FrameMs;
	}
	else if (bFull && Evicted >= WorstFrameMs)
	{
		WorstFrameMs = 0.f;
		for (int32 i = 0; i < SampleCount; ++i)
		{
			WorstFrameMs = FMath::Max(WorstFrameMs, FrameTimesMs[i]);
		}
	}

	bP95Dirty = true;
}

float UCWPerformanceSubsystem::GetP95FrameMs() const
{
	if (SampleCount == 0)
	{
		return 0.f;
	}

	if (bP95Dirty)
	{
		TArray<float> Sorted(FrameTimesMs.GetData(), SampleCount);
		Sorted.Sort();
		const int32 P95Index = FMath::Clamp(FMath::FloorToInt(0.95f * (SampleCount - 1)), 0, SampleCount - 1);
		CachedP95FrameMs = Sorted[P95Index];
		bP95Dirty = false;
	}

	return CachedP95FrameMs;
}

void UCWPerformanceSubsystem::LogReport()
{
	if (SampleCount == 0)
	{
		UE_LOG(LogCWPerf, Warning, TEXT("Sin muestras de rendimiento todavia."));
		return;
	}

	UE_LOG(LogCWPerf, Log,
		TEXT("Rendimiento: %.1f FPS | frame medio %.2f ms | p95 %.2f ms | peor %.2f ms | %d muestras"),
		AverageFPS, AverageFrameMs, GetP95FrameMs(), WorstFrameMs, SampleCount);
}

void UCWPerformanceSubsystem::ResetSamples()
{
	NextIndex = 0;
	SampleCount = 0;
	SumMs = 0.0;
	TimeSinceReport = 0.f;
	AverageFPS = 0.f;
	AverageFrameMs = 0.f;
	WorstFrameMs = 0.f;
	CachedP95FrameMs = 0.f;
	bP95Dirty = true;

	UE_LOG(LogCWPerf, Log, TEXT("Ventana de muestras de rendimiento vaciada."));
}
