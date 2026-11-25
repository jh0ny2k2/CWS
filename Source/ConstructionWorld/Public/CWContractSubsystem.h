#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWContractTypes.h"
#include "CWTerrainTypes.h"
#include "CWContractSubsystem.generated.h"

class ACWDumpTruck;
class ACWDumpZone;
class ACWExcavator;
class ACWPlayerCharacter;
class ACWWorkParcel;
class UCWSaveGame;

DECLARE_MULTICAST_DELEGATE_TwoParams(FCWContractStateChanged, ECWContractState /*Old*/, ECWContractState /*New*/);

/**
 * Contrato activo, tutorial, economia, guardado y telemetria del vertical slice (H3).
 *
 * - F4.01 maquina de estados con transiciones validadas (CWContract::IsTransitionAllowed).
 * - F4.02 cuenta solo lo excavado dentro de la ACWWorkParcel y lo entregado en ACWDumpZone.
 * - F4.04 tutorial por pasos que avanza solo y avisos comprensibles (UCWNotificationSubsystem).
 * - F4.05 recompensa menos gasoil, horas de maquina y penalizaciones; saldo persistente.
 * - F4.06 guardado en momentos seguros y restauracion integral (contrato, terreno, maquinas, jugador).
 * - F4.08 telemetria del recorrido en Saved/Telemetry/*.jsonl (cw.Telemetry 0/1).
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWContractSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* DefaultSlot;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Busca/crea la parcela y se engancha al terreno y a las zonas. Idempotente. */
	void EnsureInitialized();

	// --- Contrato ---------------------------------------------------------------
	const FCWContractDefinition& GetDefinition() const { return Definition; }
	FCWContractDefinition& GetMutableDefinition() { return Definition; }
	const FCWContractProgress& GetProgress() const { return Progress; }
	ECWContractState GetState() const { return Progress.State; }
	const FCWContractResult& GetLastResult() const { return LastResult; }
	float GetBalance() const { return Balance; }
	bool IsActive() const;
	float GetDeliveredRatio() const;

	/** Cambia de estado si la transicion es valida. */
	bool TryTransition(ECWContractState NewState, FString& OutError);

	bool AcceptContract();
	bool AbandonContract();
	/** Vuelve a Disponible y borra el progreso (el terreno no se toca). */
	void ResetContract();

	/** Coste acumulado hasta ahora (para el HUD). */
	FCWContractResult ComputeSettlement() const;

	FCWContractStateChanged OnStateChanged;

	// --- Mundo ------------------------------------------------------------------
	ACWWorkParcel* GetParcel() const { return Parcel.Get(); }
	ACWDumpZone* GetDumpZone() const;
	ACWExcavator* FindExcavator() const;
	ACWDumpTruck* FindTruck() const;
	ACWPlayerCharacter* FindPlayerCharacter() const;

	// --- Tutorial ---------------------------------------------------------------
	ECWTutorialStep GetTutorialStep() const { return TutorialStep; }
	bool AreHintsEnabled() const;
	/** Evalua el paso actual y avanza los que ya se cumplan. Devuelve true si avanzo. */
	bool UpdateTutorial();
	FString GetObjectiveText() const;
	/** Punto del mundo al que apunta el marcador del HUD para el paso actual. */
	bool GetObjectiveLocation(FVector& OutLocation, FString& OutLabel) const;

	// --- Guardado ---------------------------------------------------------------
	bool IsSafeToSave(FString& OutReason) const;
	bool SaveGameToSlot(const FString& Slot, bool bForce, FString& OutError);
	bool LoadGameFromSlot(const FString& Slot, FString& OutError);
	static bool HasSave(const FString& Slot);
	static bool DeleteSave(const FString& Slot);
	/** Guarda en DefaultSlot si es seguro; si no, lo intenta en los siguientes segundos. */
	void RequestAutosave(const FString& Reason);

	// --- Telemetria -------------------------------------------------------------
	void LogEvent(const FString& Name, const TMap<FString, FString>& Fields = TMap<FString, FString>());
	const FString& GetTelemetryFile() const { return TelemetryFile; }

	/** Desactiva el tutorial, el autoguardado y la telemetria periodica (pruebas automaticas). */
	bool bAutomationMode = false;

private:
	void HandleTerrainEdited(const FVector& Center, float DeltaM3, ECWSoilType Soil);

	UFUNCTION()
	void HandleLoadDelivered(float CubicMeters, AActor* Source);

	void SetTutorialStep(ECWTutorialStep NewStep, bool bAnnounce);
	void CompleteContract();
	void SampleFuel();
	bool IsInsideAnyDumpZone(const FVector& Location) const;

	FCWContractDefinition Definition;
	FCWContractProgress Progress;
	FCWContractResult LastResult;
	ECWTutorialStep TutorialStep = ECWTutorialStep::Briefing;
	float Balance = 25000.f;

	TWeakObjectPtr<ACWWorkParcel> Parcel;
	TArray<TWeakObjectPtr<ACWDumpZone>> DumpZones;
	FDelegateHandle TerrainHandle;
	bool bInitialized = false;

	float FuelBaselineLiters = -1.f;
	float LastFuelSample = -1.f;

	bool bAutosavePending = false;
	FString AutosaveReason;
	float AutosaveRetryTimer = 0.f;

	FString TelemetryFile;
	double TelemetryStartSeconds = 0.0;
	float TelemetrySampleTimer = 0.f;

	float LastOutsideDigNotice = -100.f;
};
