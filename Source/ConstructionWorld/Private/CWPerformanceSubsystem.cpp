#include "CWPerformanceSubsystem.h"

#include "CWLog.h"
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

void UCWPerformanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FrameTimesMs.Reserve(MaxSamples);
	TimeSinceReport = 0.f;

	ReportNowCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("cw.Perf.ReportNow"),
		TEXT("Escribe un resumen inmediato de FPS/tiempo de frame en el log."),
		FConsoleCommandDelegate::CreateUObject(this, &UCWPerformanceSubsystem::LogReport));

	ResetCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("cw.Perf.Reset"),
		TEXT("Vacia la ventana de muestras de rendimiento."),
		FConsoleCommandDelegate::CreateUObject(this, &UCWPerformanceSubsystem::ResetSamples));

	UE_LOG(LogCWPerf, Log,
		TEXT("Metricas de rendimiento activas (ventana=%d frames). Comandos: cw.Perf.Hud, cw.Perf.ReportNow, cw.Perf.Reset."),
		MaxSamples);
}

void UCWPerformanceSubsystem::Deinitialize()
{
	ReportNowCommand.Reset();
	ResetCommand.Reset();
	FrameTimesMs.Reset();

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
	if (FrameTimesMs.Num() >= MaxSamples)
	{
		FrameTimesMs.RemoveAt(0, 1, EAllowShrinking::No);
	}

	FrameTimesMs.Add(FrameMs);
	RecalculateStats();
}

void UCWPerformanceSubsystem::RecalculateStats()
{
	const int32 Count = FrameTimesMs.Num();
	if (Count == 0)
	{
		AverageFPS = 0.f;
		AverageFrameMs = 0.f;
		P95FrameMs = 0.f;
		WorstFrameMs = 0.f;
		return;
	}

	double Sum = 0.0;
	WorstFrameMs = 0.f;
	for (const float FrameMs : FrameTimesMs)
	{
		Sum += FrameMs;
		WorstFrameMs = FMath::Max(WorstFrameMs, FrameMs);
	}

	AverageFrameMs = static_cast<float>(Sum / Count);
	AverageFPS = AverageFrameMs > KINDA_SMALL_NUMBER ? (1000.f / AverageFrameMs) : 0.f;

	TArray<float> Sorted(FrameTimesMs);
	Sorted.Sort();
	const int32 P95Index = FMath::Clamp(FMath::FloorToInt(0.95f * (Count - 1)), 0, Count - 1);
	P95FrameMs = Sorted[P95Index];
}

void UCWPerformanceSubsystem::LogReport()
{
	if (FrameTimesMs.Num() == 0)
	{
		UE_LOG(LogCWPerf, Warning, TEXT("Sin muestras de rendimiento todavia."));
		return;
	}

	UE_LOG(LogCWPerf, Log,
		TEXT("Rendimiento: %.1f FPS | frame medio %.2f ms | p95 %.2f ms | peor %.2f ms | %d muestras"),
		AverageFPS, AverageFrameMs, P95FrameMs, WorstFrameMs, FrameTimesMs.Num());
}

void UCWPerformanceSubsystem::ResetSamples()
{
	FrameTimesMs.Reset();
	TimeSinceReport = 0.f;
	AverageFPS = 0.f;
	AverageFrameMs = 0.f;
	P95FrameMs = 0.f;
	WorstFrameMs = 0.f;

	UE_LOG(LogCWPerf, Log, TEXT("Ventana de muestras de rendimiento vaciada."));
}
