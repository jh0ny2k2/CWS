#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CWContractTypes.h"
#include "CWPlayerController.generated.h"

class SCWMenuWidget;
class SWidget;

UENUM()
enum class ECWMenuPage : uint8
{
	None,
	Main,
	Briefing,
	Pause,
	Settings,
	Summary
};

/**
 * Controlador del jugador (F4.07): menu principal, pausa, ajustes, resumen del contrato,
 * guardado rapido y reinicio de mision. Las teclas globales funcionan conduzca o no:
 *   Esc / P  pausa      F5  guardar      F9  cargar      J  aceptar contrato
 *
 * El menu principal no aparece con -nullrhi, -CWNoMenu ni en pruebas automaticas (-CWTest*).
 * Opciones de nivel: ?cwnew (nueva partida directa) y ?cwload (cargar la partida guardada).
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACWPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	void ShowMenu(ECWMenuPage Page);
	void CloseMenu();
	ECWMenuPage GetMenuPage() const { return CurrentPage; }
	bool IsMenuOpen() const { return CurrentPage != ECWMenuPage::None; }

	/** Esc dentro de un menu: vuelve atras o reanuda. */
	void MenuBack();

	// Acciones de los menus.
	void StartNewGame();
	void AcceptContractFromBriefing();
	void ContinueGame();
	void QuickSave();
	void QuickLoad();
	void RestartMission();
	void ReturnToMainMenu();
	void QuitGame();

	// Ajustes (persisten en GameUserSettings.ini, seccion CW.Settings).
	float GetMouseSensitivity() const;
	void SetMouseSensitivity(float Value);
	bool GetInvertY() const;
	void SetInvertY(bool bValue);
	float GetMasterVolume() const { return MasterVolume; }
	void SetMasterVolume(float Value);
	bool GetShowPerf() const;
	void SetShowPerf(bool bValue);
	bool GetShowHints() const;
	void SetShowHints(bool bValue);

	static bool ShouldShowMenusOnStart();

private:
	void TogglePause();
	void RunAutoShots();
	void AcceptContractKey();
	void LoadSettings();
	void SaveSettings() const;
	void HandleContractStateChanged(ECWContractState Old, ECWContractState New);

	TSharedPtr<SCWMenuWidget> MenuWidget;
	TSharedPtr<SWidget> MenuContainer;
	ECWMenuPage CurrentPage = ECWMenuPage::None;
	ECWMenuPage PageBeforeSettings = ECWMenuPage::Pause;
	float MasterVolume = 1.f;
	FDelegateHandle ContractHandle;

	/** Mantiene viva la secuencia de -CWAutoShot. */
	TSharedPtr<TFunction<void(int32)>> AutoShotSchedule;
};
