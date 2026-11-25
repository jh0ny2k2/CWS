#include "CWContractSubsystem.h"

#include "CWDumpTruck.h"
#include "CWDumpZone.h"
#include "CWExcavator.h"
#include "CWLog.h"
#include "CWNotificationSubsystem.h"
#include "CWPlayerCharacter.h"
#include "CWSaveGame.h"
#include "CWTerrainField.h"
#include "CWTerrainSubsystem.h"
#include "CWVehicleBase.h"
#include "CWWorkParcel.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const TCHAR* UCWContractSubsystem::DefaultSlot = TEXT("CWS_Partida");

namespace
{
	TAutoConsoleVariable<int32> CVarTelemetry(
		TEXT("cw.Telemetry"), 1,
		TEXT("Telemetria del recorrido en Saved/Telemetry (1 = si, 0 = no)."));

	TAutoConsoleVariable<int32> CVarHints(
		TEXT("cw.Tutorial.Hints"), 1,
		TEXT("Muestra los pasos del tutorial y el marcador de objetivo (1 = si, 0 = no)."));

	/** Posicion por defecto de la parcela de CWS-001 en TestLevel (delante de la excavadora). */
	const FVector DefaultParcelLocation(1400.f, -900.f, 0.f);

	FString JsonEscape(const FString& In)
	{
		FString Out = In.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
		Out.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		return Out;
	}

	FString FormatM3(float Value)
	{
		return FString::Printf(TEXT("%.1f m3"), Value);
	}
}

// --- Utilidades de tipos ------------------------------------------------------------

const TCHAR* CWContract::StateName(ECWContractState State)
{
	switch (State)
	{
	case ECWContractState::Available:  return TEXT("Disponible");
	case ECWContractState::Accepted:   return TEXT("Aceptado");
	case ECWContractState::InProgress: return TEXT("En curso");
	case ECWContractState::Completed:  return TEXT("Completado");
	case ECWContractState::Failed:     return TEXT("Fallido");
	case ECWContractState::Abandoned:  return TEXT("Abandonado");
	default:                           return TEXT("?");
	}
}

bool CWContract::IsTransitionAllowed(ECWContractState From, ECWContractState To)
{
	switch (From)
	{
	case ECWContractState::Available:
		return To == ECWContractState::Accepted;
	case ECWContractState::Accepted:
		return To == ECWContractState::InProgress || To == ECWContractState::Abandoned;
	case ECWContractState::InProgress:
		return To == ECWContractState::Completed || To == ECWContractState::Failed
			|| To == ECWContractState::Abandoned;
	case ECWContractState::Completed:
	case ECWContractState::Failed:
	case ECWContractState::Abandoned:
		return To == ECWContractState::Available;
	default:
		return false;
	}
}

FString CWContract::FormatMoney(float Euros)
{
	const int64 Rounded = FMath::RoundToInt64(FMath::Abs(Euros));
	FString Digits = FString::Printf(TEXT("%lld"), Rounded);
	for (int32 Index = Digits.Len() - 3; Index > 0; Index -= 3)
	{
		Digits.InsertAt(Index, TEXT('.'));
	}
	return FString::Printf(TEXT("%s%s EUR"), Euros < -0.5f ? TEXT("-") : TEXT(""), *Digits);
}

FString CWContract::FormatDuration(float Seconds)
{
	const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
	return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
}

// --- Ciclo de vida ------------------------------------------------------------------

bool UCWContractSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UCWContractSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Los actores aun no han hecho BeginPlay (terreno sin alturas): se inicializa en el primer Tick.
	bAutomationMode = FParse::Param(FCommandLine::Get(), TEXT("CWTestContract"));
}

void UCWContractSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
		{
			Terrain->OnTerrainEdited.Remove(TerrainHandle);
		}
	}
	Super::Deinitialize();
}

TStatId UCWContractSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCWContractSubsystem, STATGROUP_Tickables);
}

void UCWContractSubsystem::EnsureInitialized()
{
	UWorld* World = GetWorld();
	if (bInitialized || !World || !World->HasBegunPlay())
	{
		return;
	}
	bInitialized = true;

	for (TActorIterator<ACWDumpZone> It(World); It; ++It)
	{
		DumpZones.Add(*It);
		It->OnLoadDelivered.AddUniqueDynamic(this, &UCWContractSubsystem::HandleLoadDelivered);
	}

	UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>();
	if (Terrain)
	{
		TerrainHandle = Terrain->OnTerrainEdited.AddUObject(this, &UCWContractSubsystem::HandleTerrainEdited);
	}

	for (TActorIterator<ACWWorkParcel> It(World); It; ++It)
	{
		Parcel = *It;
		break;
	}

	// TestLevel aun no trae parcela: se crea sobre el terreno si hay zona de descarga (mapa de obra).
	if (!Parcel.IsValid() && DumpZones.Num() > 0 && Terrain && Terrain->GetPrimaryField())
	{
		ACWTerrainField* Field = Terrain->FindTerrainAt(DefaultParcelLocation);
		if (!Field)
		{
			Field = Terrain->GetPrimaryField();
		}
		FVector Location = Field->ContainsLocation(DefaultParcelLocation) ? DefaultParcelLocation : Field->GetFieldCenter();
		Location.Z = Field->GetHeightAtLocation(Location);

		FActorSpawnParameters Params;
		Params.Name = TEXT("CW_WorkParcel_CWS001");
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Parcel = World->SpawnActor<ACWWorkParcel>(ACWWorkParcel::StaticClass(), Location, FRotator::ZeroRotator, Params);
	}

	if (ACWWorkParcel* P = Parcel.Get())
	{
		Definition.Id = P->ContractId;
	}

	TelemetryStartSeconds = World->GetRealTimeSeconds();
	UE_LOG(LogCWContract, Log, TEXT("Contrato %s listo: parcela=%s, zonas=%d, estado=%s."),
		*Definition.Id, *GetNameSafe(Parcel.Get()), DumpZones.Num(), CWContract::StateName(Progress.State));
}

void UCWContractSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
	{
		EnsureInitialized();
		if (!bInitialized)
		{
			return;
		}
	}

	if (IsActive())
	{
		Progress.ElapsedSeconds += DeltaTime;
		SampleFuel();

		if (Definition.TimeLimitSeconds > 0.f && Progress.State == ECWContractState::InProgress
			&& Progress.ElapsedSeconds > Definition.TimeLimitSeconds)
		{
			FString Error;
			if (TryTransition(ECWContractState::Failed, Error))
			{
				UCWNotificationSubsystem::Notify(this,
					FString::Printf(TEXT("Se ha agotado el plazo de %s. El contrato se da por fallido."),
						*CWContract::FormatDuration(Definition.TimeLimitSeconds)), ECWNoticeSeverity::Error);
			}
		}

		if (!bAutomationMode)
		{
			UpdateTutorial();

			TelemetrySampleTimer += DeltaTime;
			if (TelemetrySampleTimer >= 5.f)
			{
				TelemetrySampleTimer = 0.f;
				const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
				const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
				if (Pawn)
				{
					LogEvent(TEXT("pos"), {
						{ TEXT("pawn"), Pawn->GetClass()->GetName() },
						{ TEXT("x"), FString::Printf(TEXT("%.0f"), Pawn->GetActorLocation().X) },
						{ TEXT("y"), FString::Printf(TEXT("%.0f"), Pawn->GetActorLocation().Y) },
						{ TEXT("z"), FString::Printf(TEXT("%.0f"), Pawn->GetActorLocation().Z) } });
				}
			}
		}
	}

	if (bAutosavePending)
	{
		AutosaveRetryTimer -= DeltaTime;
		if (AutosaveRetryTimer <= 0.f)
		{
			FString Error;
			if (SaveGameToSlot(DefaultSlot, false, Error))
			{
				bAutosavePending = false;
				UCWNotificationSubsystem::Notify(this, TEXT("Partida guardada automaticamente."), ECWNoticeSeverity::Info);
			}
			else
			{
				AutosaveRetryTimer = 3.f;
			}
		}
	}
}

// --- Contrato -----------------------------------------------------------------------

bool UCWContractSubsystem::IsActive() const
{
	return Progress.State == ECWContractState::Accepted || Progress.State == ECWContractState::InProgress;
}

float UCWContractSubsystem::GetDeliveredRatio() const
{
	return Definition.TargetVolumeM3 > 0.f ? FMath::Clamp(Progress.DeliveredM3 / Definition.TargetVolumeM3, 0.f, 1.f) : 0.f;
}

bool UCWContractSubsystem::TryTransition(ECWContractState NewState, FString& OutError)
{
	const ECWContractState Old = Progress.State;
	if (!CWContract::IsTransitionAllowed(Old, NewState))
	{
		OutError = FString::Printf(TEXT("transicion no permitida: %s -> %s"),
			CWContract::StateName(Old), CWContract::StateName(NewState));
		UE_LOG(LogCWContract, Warning, TEXT("Contrato %s: %s"), *Definition.Id, *OutError);
		return false;
	}

	Progress.State = NewState;
	UE_LOG(LogCWContract, Log, TEXT("Contrato %s: %s -> %s"), *Definition.Id,
		CWContract::StateName(Old), CWContract::StateName(NewState));
	LogEvent(TEXT("contract_state"), { { TEXT("from"), CWContract::StateName(Old) }, { TEXT("to"), CWContract::StateName(NewState) } });
	OnStateChanged.Broadcast(Old, NewState);
	return true;
}

bool UCWContractSubsystem::AcceptContract()
{
	EnsureInitialized();
	FString Error;
	if (!TryTransition(ECWContractState::Accepted, Error))
	{
		return false;
	}

	FuelBaselineLiters = -1.f;
	LastFuelSample = -1.f;
	SampleFuel();
	SetTutorialStep(ECWTutorialStep::GoToExcavator, false);
	UCWNotificationSubsystem::Notify(this,
		FString::Printf(TEXT("Contrato %s aceptado: entrega %s en la zona de descarga."),
			*Definition.Id, *FormatM3(Definition.TargetVolumeM3)), ECWNoticeSeverity::Success);
	RequestAutosave(TEXT("contrato aceptado"));
	return true;
}

bool UCWContractSubsystem::AbandonContract()
{
	FString Error;
	if (!TryTransition(ECWContractState::Abandoned, Error))
	{
		return false;
	}
	UCWNotificationSubsystem::Notify(this, TEXT("Has abandonado el contrato."), ECWNoticeSeverity::Warning);
	return true;
}

void UCWContractSubsystem::ResetContract()
{
	const ECWContractState Old = Progress.State;
	Progress = FCWContractProgress();
	LastResult = FCWContractResult();
	TutorialStep = ECWTutorialStep::Briefing;
	FuelBaselineLiters = -1.f;
	LastFuelSample = -1.f;
	if (Old != ECWContractState::Available)
	{
		LogEvent(TEXT("contract_reset"));
		OnStateChanged.Broadcast(Old, Progress.State);
	}
}

FCWContractResult UCWContractSubsystem::ComputeSettlement() const
{
	FCWContractResult Result;
	Result.bValid = true;
	Result.ElapsedSeconds = Progress.ElapsedSeconds;
	Result.Reward = Progress.State == ECWContractState::Completed || Progress.DeliveredM3 >= Definition.TargetVolumeM3
		? Definition.Reward : 0.f;
	Result.FuelCost = Progress.FuelUsedLiters * Definition.FuelPricePerLiter;
	Result.MachineCost = Progress.ElapsedSeconds / 3600.f * Definition.MachineHourlyRate;
	Result.Penalties = Progress.OutsideDigM3 * Definition.OutsideDigPenaltyPerM3
		+ Progress.IllegalDumpM3 * Definition.IllegalDumpPenaltyPerM3;
	Result.Net = Result.Reward - Result.FuelCost - Result.MachineCost - Result.Penalties;
	return Result;
}

void UCWContractSubsystem::CompleteContract()
{
	SampleFuel();
	FString Error;
	if (Progress.State == ECWContractState::Accepted)
	{
		TryTransition(ECWContractState::InProgress, Error);
	}
	if (!TryTransition(ECWContractState::Completed, Error))
	{
		return;
	}

	LastResult = ComputeSettlement();
	Balance += LastResult.Net;
	SetTutorialStep(ECWTutorialStep::Finished, false);

	UCWNotificationSubsystem::Notify(this,
		FString::Printf(TEXT("Contrato %s completado. Beneficio neto: %s."),
			*Definition.Id, *CWContract::FormatMoney(LastResult.Net)), ECWNoticeSeverity::Success);

	LogEvent(TEXT("contract_complete"), {
		{ TEXT("seconds"), FString::Printf(TEXT("%.1f"), LastResult.ElapsedSeconds) },
		{ TEXT("reward"), FString::Printf(TEXT("%.2f"), LastResult.Reward) },
		{ TEXT("fuel"), FString::Printf(TEXT("%.2f"), LastResult.FuelCost) },
		{ TEXT("machines"), FString::Printf(TEXT("%.2f"), LastResult.MachineCost) },
		{ TEXT("penalties"), FString::Printf(TEXT("%.2f"), LastResult.Penalties) },
		{ TEXT("net"), FString::Printf(TEXT("%.2f"), LastResult.Net) },
		{ TEXT("trips"), FString::FromInt(Progress.Trips) } });

	RequestAutosave(TEXT("contrato completado"));
}

void UCWContractSubsystem::SampleFuel()
{
	const ACWExcavator* Excavator = FindExcavator();
	if (!Excavator)
	{
		return;
	}

	const float Fuel = Excavator->GetFuelLiters();
	if (FuelBaselineLiters < 0.f || LastFuelSample < 0.f)
	{
		FuelBaselineLiters = Fuel;
		LastFuelSample = Fuel;
		return;
	}

	// Solo cuenta el consumo (un repostaje no "devuelve" gasoil gastado).
	if (Fuel < LastFuelSample)
	{
		Progress.FuelUsedLiters += LastFuelSample - Fuel;
	}
	LastFuelSample = Fuel;
}

// --- Eventos de juego ---------------------------------------------------------------

bool UCWContractSubsystem::IsInsideAnyDumpZone(const FVector& Location) const
{
	for (const TWeakObjectPtr<ACWDumpZone>& Zone : DumpZones)
	{
		if (Zone.IsValid() && Zone->ZoneBox)
		{
			const FBox Box = Zone->ZoneBox->Bounds.GetBox().ExpandBy(FVector(150.f, 150.f, 1000.f));
			if (Box.IsInsideXY(Location))
			{
				return true;
			}
		}
	}
	return false;
}

void UCWContractSubsystem::HandleTerrainEdited(const FVector& Center, float DeltaM3, ECWSoilType Soil)
{
	if (!IsActive())
	{
		return;
	}

	const bool bInParcel = Parcel.IsValid() && Parcel->ContainsLocation(Center);
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	if (DeltaM3 < 0.f)
	{
		const float Dug = -DeltaM3;
		if (bInParcel)
		{
			Progress.ExcavatedM3 += Dug;
			if (Progress.State == ECWContractState::Accepted)
			{
				FString Error;
				TryTransition(ECWContractState::InProgress, Error);
			}
		}
		else
		{
			Progress.OutsideDigM3 += Dug;
			if (Now - LastOutsideDigNotice > 6.f)
			{
				LastOutsideDigNotice = Now;
				UCWNotificationSubsystem::Notify(this,
					FString::Printf(TEXT("Estas excavando fuera de la parcela balizada: no cuenta para %s y se penaliza con %.0f EUR/m3."),
						*Definition.Id, Definition.OutsideDigPenaltyPerM3), ECWNoticeSeverity::Warning);
			}
		}
		LogEvent(TEXT("dig"), { { TEXT("m3"), FString::Printf(TEXT("%.2f"), Dug) }, { TEXT("inParcel"), bInParcel ? TEXT("1") : TEXT("0") } });
		return;
	}

	// Deposito: devuelto a la parcela, entregado en zona (lo gestiona HandleLoadDelivered) o vertido ilegal.
	if (bInParcel)
	{
		Progress.ExcavatedM3 = FMath::Max(0.f, Progress.ExcavatedM3 - DeltaM3);
		UCWNotificationSubsystem::Notify(this,
			TEXT("Has devuelto tierra a la parcela: se descuenta de lo excavado."), ECWNoticeSeverity::Info);
	}
	else if (!IsInsideAnyDumpZone(Center))
	{
		Progress.IllegalDumpM3 += DeltaM3;
		UCWNotificationSubsystem::Notify(this,
			FString::Printf(TEXT("Has vertido %.1f m3 fuera de la zona de descarga: vertido no autorizado (%.0f EUR/m3)."),
				DeltaM3, Definition.IllegalDumpPenaltyPerM3), ECWNoticeSeverity::Error);
	}
	LogEvent(TEXT("deposit"), { { TEXT("m3"), FString::Printf(TEXT("%.2f"), DeltaM3) }, { TEXT("inParcel"), bInParcel ? TEXT("1") : TEXT("0") } });
}

void UCWContractSubsystem::HandleLoadDelivered(float CubicMeters, AActor* Source)
{
	if (!IsActive() || CubicMeters <= 0.f)
	{
		return;
	}

	Progress.DeliveredM3 += CubicMeters;
	++Progress.Trips;
	LogEvent(TEXT("delivered"), { { TEXT("m3"), FString::Printf(TEXT("%.2f"), CubicMeters) }, { TEXT("total"), FString::Printf(TEXT("%.2f"), Progress.DeliveredM3) } });

	if (Progress.State == ECWContractState::Accepted)
	{
		FString Error;
		TryTransition(ECWContractState::InProgress, Error);
	}

	if (Progress.DeliveredM3 + KINDA_SMALL_NUMBER >= Definition.TargetVolumeM3)
	{
		CompleteContract();
		return;
	}

	const float Remaining = Definition.TargetVolumeM3 - Progress.DeliveredM3;
	UCWNotificationSubsystem::Notify(this,
		FString::Printf(TEXT("Entregados %.1f m3. Faltan %.1f m3: vuelve a cargar en la parcela."), CubicMeters, Remaining),
		ECWNoticeSeverity::Success);
	if (TutorialStep == ECWTutorialStep::Dump || TutorialStep == ECWTutorialStep::DriveToZone)
	{
		SetTutorialStep(ECWTutorialStep::GoToExcavator, false);
	}
	RequestAutosave(TEXT("entrega"));
}

// --- Mundo --------------------------------------------------------------------------

ACWDumpZone* UCWContractSubsystem::GetDumpZone() const
{
	for (const TWeakObjectPtr<ACWDumpZone>& Zone : DumpZones)
	{
		if (Zone.IsValid())
		{
			return Zone.Get();
		}
	}
	return nullptr;
}

ACWExcavator* UCWContractSubsystem::FindExcavator() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACWExcavator> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

ACWDumpTruck* UCWContractSubsystem::FindTruck() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACWDumpTruck> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

ACWPlayerCharacter* UCWContractSubsystem::FindPlayerCharacter() const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (ACWPlayerCharacter* Character = Cast<ACWPlayerCharacter>(Pawn))
	{
		return Character;
	}
	if (const ACWVehicleBase* Vehicle = Cast<ACWVehicleBase>(Pawn))
	{
		return Vehicle->GetDriver();
	}
	return nullptr;
}

// --- Tutorial -----------------------------------------------------------------------

bool UCWContractSubsystem::AreHintsEnabled() const
{
	return CVarHints.GetValueOnGameThread() != 0;
}

void UCWContractSubsystem::SetTutorialStep(ECWTutorialStep NewStep, bool bAnnounce)
{
	if (TutorialStep == NewStep)
	{
		return;
	}

	const ECWTutorialStep Old = TutorialStep;
	TutorialStep = NewStep;
	LogEvent(TEXT("tutorial_step"), {
		{ TEXT("from"), FString::FromInt(static_cast<int32>(Old)) },
		{ TEXT("to"), FString::FromInt(static_cast<int32>(NewStep)) } });

	if (bAnnounce && AreHintsEnabled())
	{
		UCWNotificationSubsystem::Notify(this, FString::Printf(TEXT("Hecho. Siguiente: %s"), *GetObjectiveText()),
			ECWNoticeSeverity::Success);
	}

	// Los cambios de paso son momentos naturales de guardado (si es seguro).
	if (NewStep == ECWTutorialStep::DigInParcel || NewStep == ECWTutorialStep::DriveToZone)
	{
		RequestAutosave(TEXT("paso del tutorial"));
	}
}

bool UCWContractSubsystem::UpdateTutorial()
{
	if (!IsActive())
	{
		return false;
	}

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const ACWExcavator* Excavator = FindExcavator();
	const ACWDumpTruck* Truck = FindTruck();
	const ACWPlayerCharacter* Character = FindPlayerCharacter();

	const float Remaining = FMath::Max(0.f, Definition.TargetVolumeM3 - Progress.DeliveredM3);
	const float TruckWanted = Truck ? FMath::Min(Remaining, Truck->GetCapacityCubicMeters()) : Remaining;
	const float TruckLoad = Truck ? Truck->GetLoadCubicMeters() : 0.f;
	const bool bTruckReady = Truck && TruckLoad + 0.6f >= TruckWanted;

	bool bAdvanced = false;
	// Varias iteraciones: si el jugador ya cumple pasos posteriores se saltan seguidos.
	for (int32 Guard = 0; Guard < 10; ++Guard)
	{
		ECWTutorialStep Next = TutorialStep;
		switch (TutorialStep)
		{
		case ECWTutorialStep::Briefing:
			Next = ECWTutorialStep::GoToExcavator;
			break;
		case ECWTutorialStep::GoToExcavator:
			if (bTruckReady)
			{
				Next = ECWTutorialStep::EnterTruck;
			}
			else if (Excavator && (Pawn == Excavator
				|| (Character && FVector::Dist2D(Character->GetActorLocation(), Excavator->GetActorLocation()) < 800.f)))
			{
				Next = ECWTutorialStep::EnterExcavator;
			}
			break;
		case ECWTutorialStep::EnterExcavator:
			if (Excavator && Pawn == Excavator)
			{
				Next = ECWTutorialStep::StartEngine;
			}
			break;
		case ECWTutorialStep::StartEngine:
			if (Excavator && Excavator->IsEngineRunning())
			{
				Next = ECWTutorialStep::DigMode;
			}
			break;
		case ECWTutorialStep::DigMode:
			if (Excavator && Excavator->GetControlMode() == ECWExcavatorControlMode::Dig)
			{
				Next = ECWTutorialStep::DigInParcel;
			}
			break;
		case ECWTutorialStep::DigInParcel:
			if (Progress.ExcavatedM3 > 0.4f || TruckLoad > 0.1f)
			{
				Next = ECWTutorialStep::LoadTruck;
			}
			break;
		case ECWTutorialStep::LoadTruck:
			if (bTruckReady)
			{
				Next = ECWTutorialStep::EnterTruck;
			}
			break;
		case ECWTutorialStep::EnterTruck:
			if (Truck && Pawn == Truck)
			{
				Next = ECWTutorialStep::DriveToZone;
			}
			break;
		case ECWTutorialStep::DriveToZone:
			if (Truck && Truck->IsInsideDumpZone())
			{
				Next = ECWTutorialStep::Dump;
			}
			break;
		case ECWTutorialStep::Dump:
			// Avanza en HandleLoadDelivered.
			break;
		case ECWTutorialStep::Finished:
		default:
			break;
		}

		if (Next == TutorialStep)
		{
			break;
		}
		SetTutorialStep(Next, true);
		bAdvanced = true;
	}
	return bAdvanced;
}

FString UCWContractSubsystem::GetObjectiveText() const
{
	const ACWDumpTruck* Truck = FindTruck();
	const float Remaining = FMath::Max(0.f, Definition.TargetVolumeM3 - Progress.DeliveredM3);
	const float TruckWanted = Truck ? FMath::Min(Remaining, Truck->GetCapacityCubicMeters()) : Remaining;

	switch (Progress.State)
	{
	case ECWContractState::Available:
		return FString::Printf(TEXT("Acepta el contrato %s (tecla J o menu)."), *Definition.Id);
	case ECWContractState::Completed:
		return TEXT("Contrato completado. Revisa la liquidacion.");
	case ECWContractState::Failed:
		return TEXT("Contrato fallido. Reinicia la mision desde el menu de pausa.");
	case ECWContractState::Abandoned:
		return TEXT("Contrato abandonado. Reinicia la mision desde el menu de pausa.");
	default:
		break;
	}

	switch (TutorialStep)
	{
	case ECWTutorialStep::Briefing:
	case ECWTutorialStep::GoToExcavator:
		return TEXT("Camina hasta la excavadora (sigue el marcador).");
	case ECWTutorialStep::EnterExcavator:
		return TEXT("Pulsa E junto a la excavadora para subir.");
	case ECWTutorialStep::StartEngine:
		return TEXT("Pulsa G para arrancar el motor.");
	case ECWTutorialStep::DigMode:
		return TEXT("Acerca la maquina a la parcela con WASD y pulsa R para pasar a modo excavacion.");
	case ECWTutorialStep::DigInParcel:
		return TEXT("Baja la cuchara a la parcela (W/S pluma, A/D brazo, Z/X cuchara) y pulsa T para excavar.");
	case ECWTutorialStep::LoadTruck:
		return FString::Printf(TEXT("Gira con Q/E hasta el volquete y pulsa T para vaciar la cuchara. Carga %.1f / %.1f m3."),
			Truck ? Truck->GetLoadCubicMeters() : 0.f, TruckWanted);
	case ECWTutorialStep::EnterTruck:
		return TEXT("Baja con F y sube al volquete con E.");
	case ECWTutorialStep::DriveToZone:
		return TEXT("Conduce hasta la zona de descarga (sigue el marcador). Sin prisa: a mas velocidad, mas derrame.");
	case ECWTutorialStep::Dump:
		return TEXT("Detente, sube la caja con B y pulsa T para descargar.");
	case ECWTutorialStep::Finished:
	default:
		return TEXT("Trabajo terminado.");
	}
}

bool UCWContractSubsystem::GetObjectiveLocation(FVector& OutLocation, FString& OutLabel) const
{
	if (!IsActive())
	{
		return false;
	}

	switch (TutorialStep)
	{
	case ECWTutorialStep::Briefing:
	case ECWTutorialStep::GoToExcavator:
	case ECWTutorialStep::EnterExcavator:
		if (const ACWExcavator* Excavator = FindExcavator())
		{
			OutLocation = Excavator->GetActorLocation() + FVector(0.f, 0.f, 350.f);
			OutLabel = TEXT("Excavadora");
			return true;
		}
		break;
	case ECWTutorialStep::StartEngine:
	case ECWTutorialStep::DigMode:
	case ECWTutorialStep::DigInParcel:
		if (const ACWWorkParcel* P = Parcel.Get())
		{
			OutLocation = P->GetActorLocation() + FVector(0.f, 0.f, 250.f);
			OutLabel = TEXT("Parcela");
			return true;
		}
		break;
	case ECWTutorialStep::LoadTruck:
	case ECWTutorialStep::EnterTruck:
		if (const ACWDumpTruck* Truck = FindTruck())
		{
			OutLocation = Truck->GetActorLocation() + FVector(0.f, 0.f, 350.f);
			OutLabel = TEXT("Volquete");
			return true;
		}
		break;
	case ECWTutorialStep::DriveToZone:
	case ECWTutorialStep::Dump:
		if (const ACWDumpZone* Zone = GetDumpZone())
		{
			OutLocation = Zone->GetActorLocation() + FVector(0.f, 0.f, 200.f);
			OutLabel = TEXT("Zona de descarga");
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

// --- Guardado -----------------------------------------------------------------------

bool UCWContractSubsystem::IsSafeToSave(FString& OutReason) const
{
	if (const ACWDumpTruck* Truck = FindTruck())
	{
		if (Truck->GetBedAngle() > 5.f)
		{
			OutReason = TEXT("baja la caja del volquete antes de guardar");
			return false;
		}
		if (FMath::Abs(Truck->GetSpeedCmS()) > 60.f)
		{
			OutReason = TEXT("deten el volquete antes de guardar");
			return false;
		}
	}

	if (const ACWExcavator* Excavator = FindExcavator())
	{
		if (FMath::Abs(Excavator->GetLeftTrackSpeed()) > 30.f || FMath::Abs(Excavator->GetRightTrackSpeed()) > 30.f)
		{
			OutReason = TEXT("deten la excavadora antes de guardar");
			return false;
		}
	}

	if (const ACWPlayerCharacter* Character = FindPlayerCharacter())
	{
		if (Character->GetVelocity().Z < -200.f)
		{
			OutReason = TEXT("no se puede guardar mientras caes");
			return false;
		}
	}

	return true;
}

bool UCWContractSubsystem::SaveGameToSlot(const FString& Slot, bool bForce, FString& OutError)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutError = TEXT("no hay mundo");
		return false;
	}

	if (!bForce && !IsSafeToSave(OutError))
	{
		return false;
	}

	UCWSaveGame* Save = Cast<UCWSaveGame>(UGameplayStatics::CreateSaveGameObject(UCWSaveGame::StaticClass()));
	Save->MapName = UGameplayStatics::GetCurrentLevelName(this, true);
	Save->SavedAt = FDateTime::Now();
	Save->ContractId = Definition.Id;
	Save->Progress = Progress;
	Save->LastResult = LastResult;
	Save->TutorialStep = TutorialStep;
	Save->Balance = Balance;
	Save->FuelBaselineLiters = FuelBaselineLiters;

	if (const ACWPlayerCharacter* Character = FindPlayerCharacter())
	{
		// Si conduce, se guarda junto al vehiculo: al cargar aparece a pie al lado.
		FTransform PlayerTransform = Character->GetActorTransform();
		if (const ACWVehicleBase* Vehicle = Cast<ACWVehicleBase>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			PlayerTransform.SetLocation(Vehicle->GetActorLocation() - Vehicle->GetActorRightVector() * 300.f + FVector(0.f, 0.f, 50.f));
		}
		Save->PlayerTransform = PlayerTransform;
	}

	for (TActorIterator<ACWVehicleBase> It(World); It; ++It)
	{
		FCWVehicleSaveData Data;
		Data.ActorName = It->GetName();
		Data.Transform = It->GetActorTransform();
		if (const ACWExcavator* Excavator = Cast<ACWExcavator>(*It))
		{
			Data.bEngineRunning = Excavator->IsEngineRunning();
			Data.FuelLiters = Excavator->GetFuelLiters();
			Data.LoadM3 = Excavator->GetBucketLoadCubicMeters();
			Data.SoilType = Excavator->GetBucketSoilType();
		}
		else if (const ACWDumpTruck* Truck = Cast<ACWDumpTruck>(*It))
		{
			Data.LoadM3 = Truck->GetLoadCubicMeters();
			Data.SoilType = Truck->GetLoadSoilType();
		}
		Save->Vehicles.Add(Data);
	}

	if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
	{
		if (Terrain->GetPrimaryField())
		{
			Save->TerrainFile = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Slot + TEXT("_terreno.cwt")));
			FString TerrainError;
			if (!Terrain->SaveTerrain(Save->TerrainFile, TerrainError))
			{
				OutError = FString::Printf(TEXT("no se pudo guardar el terreno: %s"), *TerrainError);
				return false;
			}
		}
	}

	if (!UGameplayStatics::SaveGameToSlot(Save, Slot, 0))
	{
		OutError = TEXT("no se pudo escribir la partida");
		return false;
	}

	LogEvent(TEXT("save"), { { TEXT("slot"), Slot } });
	UE_LOG(LogCWContract, Log, TEXT("Partida guardada en '%s' (%s, %d vehiculos)."),
		*Slot, CWContract::StateName(Progress.State), Save->Vehicles.Num());
	return true;
}

bool UCWContractSubsystem::LoadGameFromSlot(const FString& Slot, FString& OutError)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutError = TEXT("no hay mundo");
		return false;
	}
	EnsureInitialized();

	UCWSaveGame* Save = Cast<UCWSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Save)
	{
		OutError = TEXT("no hay partida guardada");
		return false;
	}
	if (Save->Version > UCWSaveGame::CurrentVersion)
	{
		OutError = TEXT("la partida es de una version mas nueva del juego");
		return false;
	}

	// Primero bajar al jugador de cualquier vehiculo para poder recolocarlo todo.
	if (ACWVehicleBase* Vehicle = Cast<ACWVehicleBase>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Vehicle->ExitVehicle();
	}

	if (!Save->TerrainFile.IsEmpty())
	{
		if (UCWTerrainSubsystem* Terrain = World->GetSubsystem<UCWTerrainSubsystem>())
		{
			FString TerrainError;
			if (!Terrain->LoadTerrain(Save->TerrainFile, TerrainError))
			{
				OutError = FString::Printf(TEXT("terreno: %s"), *TerrainError);
				return false;
			}
		}
	}

	for (TActorIterator<ACWVehicleBase> It(World); It; ++It)
	{
		const FCWVehicleSaveData* Data = Save->Vehicles.FindByPredicate(
			[&It](const FCWVehicleSaveData& Entry) { return Entry.ActorName == It->GetName(); });
		if (!Data)
		{
			continue;
		}

		It->SetActorTransform(Data->Transform, false, nullptr, ETeleportType::TeleportPhysics);
		if (ACWExcavator* Excavator = Cast<ACWExcavator>(*It))
		{
			if (Data->FuelLiters >= 0.f)
			{
				Excavator->SetFuelLiters(Data->FuelLiters);
			}
			Excavator->SetBucketLoadForTest(Data->LoadM3);
			if (Data->bEngineRunning != Excavator->IsEngineRunning())
			{
				Data->bEngineRunning ? Excavator->StartEngine() : Excavator->StopEngine();
			}
		}
		else if (ACWDumpTruck* Truck = Cast<ACWDumpTruck>(*It))
		{
			Truck->RemoveLoad(Truck->GetLoadCubicMeters());
			Truck->SetLoadSoilType(Data->SoilType);
			Truck->AddLoad(Data->LoadM3);
			Truck->SetBedAngleForTest(0.f);
		}
	}

	if (ACWPlayerCharacter* Character = FindPlayerCharacter())
	{
		Character->SetActorLocationAndRotation(Save->PlayerTransform.GetLocation(),
			Save->PlayerTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	const ECWContractState Old = Progress.State;
	Definition.Id = Save->ContractId.IsEmpty() ? Definition.Id : Save->ContractId;
	Progress = Save->Progress;
	LastResult = Save->LastResult;
	TutorialStep = Save->TutorialStep;
	Balance = Save->Balance;
	FuelBaselineLiters = Save->FuelBaselineLiters;
	LastFuelSample = -1.f;
	if (const ACWExcavator* Excavator = FindExcavator())
	{
		LastFuelSample = Excavator->GetFuelLiters();
	}

	LogEvent(TEXT("load"), { { TEXT("slot"), Slot } });
	if (Old != Progress.State)
	{
		OnStateChanged.Broadcast(Old, Progress.State);
	}
	UE_LOG(LogCWContract, Log, TEXT("Partida '%s' cargada (%s, entregado %.1f m3, guardada %s)."),
		*Slot, CWContract::StateName(Progress.State), Progress.DeliveredM3, *Save->SavedAt.ToString());
	return true;
}

bool UCWContractSubsystem::HasSave(const FString& Slot)
{
	return UGameplayStatics::DoesSaveGameExist(Slot, 0);
}

bool UCWContractSubsystem::DeleteSave(const FString& Slot)
{
	return UGameplayStatics::DeleteGameInSlot(Slot, 0);
}

void UCWContractSubsystem::RequestAutosave(const FString& Reason)
{
	if (bAutomationMode)
	{
		return;
	}
	bAutosavePending = true;
	AutosaveReason = Reason;
	AutosaveRetryTimer = 0.5f;
}

// --- Telemetria ---------------------------------------------------------------------

void UCWContractSubsystem::LogEvent(const FString& Name, const TMap<FString, FString>& Fields)
{
	if (CVarTelemetry.GetValueOnGameThread() == 0)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (TelemetryFile.IsEmpty())
	{
		TelemetryFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Telemetry"),
			FString::Printf(TEXT("%s_%s.jsonl"), *Definition.Id, *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")))));
	}

	FString Line = FString::Printf(TEXT("{\"t\":%.2f,\"event\":\"%s\",\"contract\":\"%s\",\"state\":\"%s\",\"step\":%d"),
		World ? World->GetRealTimeSeconds() - TelemetryStartSeconds : 0.0,
		*JsonEscape(Name), *JsonEscape(Definition.Id), CWContract::StateName(Progress.State),
		static_cast<int32>(TutorialStep));
	for (const TPair<FString, FString>& Field : Fields)
	{
		Line += FString::Printf(TEXT(",\"%s\":\"%s\""), *JsonEscape(Field.Key), *JsonEscape(Field.Value));
	}
	Line += TEXT("}\n");

	FFileHelper::SaveStringToFile(Line, *TelemetryFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}
