#include "CWPlayerController.h"

#include "CWContractSubsystem.h"
#include "CWLog.h"
#include "CWMenuWidget.h"
#include "CWNotificationSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Containers/Ticker.h"
#include "Misc/ConfigCacheIni.h"
#include "UnrealClient.h"
#include "Widgets/SWeakWidget.h"

namespace
{
	const TCHAR* SettingsSection = TEXT("CW.Settings");

	IConsoleVariable* FindCVar(const TCHAR* Name)
	{
		return IConsoleManager::Get().FindConsoleVariable(Name);
	}
}

ACWPlayerController::ACWPlayerController()
{
	bShouldPerformFullTickWhenPaused = false;
}

bool ACWPlayerController::ShouldShowMenusOnStart()
{
	const TCHAR* Cmd = FCommandLine::Get();
	if (FApp::IsUnattended() || IsRunningCommandlet() || !FApp::CanEverRender())
	{
		return false;
	}
	if (FParse::Param(Cmd, TEXT("CWNoMenu")) || FParse::Param(Cmd, TEXT("nullrhi")))
	{
		return false;
	}
	// Cualquier prueba automatica (-CWTestEnterExit=, -CWTestTruck, ...) desactiva los menus.
	return FCString::Stristr(Cmd, TEXT("-CWTest")) == nullptr;
}

void ACWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	LoadSettings();

	UWorld* World = GetWorld();
	UCWContractSubsystem* Contract = World ? World->GetSubsystem<UCWContractSubsystem>() : nullptr;
	if (Contract)
	{
		ContractHandle = Contract->OnStateChanged.AddUObject(this, &ACWPlayerController::HandleContractStateChanged);
	}

	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	const FString Options = GameMode ? GameMode->OptionsString : FString();

	// Se difiere un frame: el terreno y el contrato se preparan en su primer tick.
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Options]()
	{
		UCWContractSubsystem* C = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr;
		if (C)
		{
			C->EnsureInitialized();
		}

		if (UGameplayStatics::HasOption(Options, TEXT("cwnew")))
		{
			if (C)
			{
				UCWContractSubsystem::DeleteSave(UCWContractSubsystem::DefaultSlot);
				C->AcceptContract();
			}
		}
		else if (UGameplayStatics::HasOption(Options, TEXT("cwload")))
		{
			FString Error;
			if (C && !C->LoadGameFromSlot(UCWContractSubsystem::DefaultSlot, Error))
			{
				UCWNotificationSubsystem::Notify(this, FString::Printf(TEXT("No se pudo cargar: %s."), *Error), ECWNoticeSeverity::Error);
			}
		}
		else if (ShouldShowMenusOnStart())
		{
			ShowMenu(ECWMenuPage::Main);
		}
	}), 0.2f, false);

	if (FParse::Param(FCommandLine::Get(), TEXT("CWAutoShot")))
	{
		RunAutoShots();
	}
}

void ACWPlayerController::RunAutoShots()
{
	// Secuencia de capturas para revisar menus y HUD sin intervencion (-CWAutoShot).
	// Usa el ticker del motor (tiempo real): los temporizadores del mundo se paran en pausa.
	struct FShotStep { float Delay; TFunction<void(ACWPlayerController*)> Action; };
	const TArray<FShotStep> Steps = {
		{ 4.f, [](ACWPlayerController* PC) { PC->ShowMenu(ECWMenuPage::Main); } },
		{ 1.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_01_menu_principal"), true, false); } },
		{ 1.f, [](ACWPlayerController* PC) { PC->ShowMenu(ECWMenuPage::Briefing); } },
		{ 1.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_02_informe_contrato"), true, false); } },
		{ 1.f, [](ACWPlayerController* PC) { PC->AcceptContractFromBriefing(); } },
		{ 3.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_03_hud_mision"), true, false); } },
		{ 1.f, [](ACWPlayerController* PC) { PC->ShowMenu(ECWMenuPage::Pause); } },
		{ 1.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_04_pausa"), true, false); } },
		{ 1.f, [](ACWPlayerController* PC) { PC->ShowMenu(ECWMenuPage::Settings); } },
		{ 1.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_05_ajustes"), true, false); } },
		{ 1.f, [](ACWPlayerController* PC) { PC->ShowMenu(ECWMenuPage::Summary); } },
		{ 1.f, [](ACWPlayerController*) { FScreenshotRequest::RequestScreenshot(TEXT("CW_06_liquidacion"), true, false); } },
		{ 2.f, [](ACWPlayerController* PC) { PC->QuitGame(); } },
	};

	// Encadenado: cada paso programa el siguiente al ejecutarse, asi un tiron de carga
	// (compilacion de shaders) no hace que se disparen varios pasos en el mismo frame.
	TWeakObjectPtr<ACWPlayerController> Weak(this);
	TSharedRef<TFunction<void(int32)>> Schedule = MakeShared<TFunction<void(int32)>>();
	TWeakPtr<TFunction<void(int32)>> WeakSchedule = Schedule;
	*Schedule = [Weak, Steps, WeakSchedule](int32 Index)
	{
		if (!Steps.IsValidIndex(Index))
		{
			return;
		}
		TSharedPtr<TFunction<void(int32)>> Self = WeakSchedule.Pin();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Steps, Index, Self](float)
		{
			if (ACWPlayerController* PC = Weak.Get())
			{
				Steps[Index].Action(PC);
				if (Self.IsValid())
				{
					(*Self)(Index + 1);
				}
			}
			return false;
		}), Steps[Index].Delay);
	};
	(*Schedule)(0);
	AutoShotSchedule = Schedule;
}

void ACWPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuContainer.ToSharedRef());
	}
	MenuContainer.Reset();
	MenuWidget.Reset();

	if (UWorld* World = GetWorld())
	{
		if (UCWContractSubsystem* Contract = World->GetSubsystem<UCWContractSubsystem>())
		{
			Contract->OnStateChanged.Remove(ContractHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ACWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACWPlayerController::TogglePause).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ACWPlayerController::TogglePause).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ACWPlayerController::TogglePause).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ACWPlayerController::QuickSave);
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ACWPlayerController::QuickLoad);
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ACWPlayerController::AcceptContractKey);
}

// --- Menus --------------------------------------------------------------------------

void ACWPlayerController::ShowMenu(ECWMenuPage Page)
{
	if (Page == ECWMenuPage::None)
	{
		CloseMenu();
		return;
	}
	if (!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized())
	{
		return;
	}

	if (Page == ECWMenuPage::Settings && CurrentPage != ECWMenuPage::Settings)
	{
		PageBeforeSettings = CurrentPage == ECWMenuPage::None ? ECWMenuPage::Pause : CurrentPage;
	}

	if (!MenuWidget.IsValid())
	{
		MenuWidget = SNew(SCWMenuWidget).Owner(this);
		MenuContainer = SNew(SWeakWidget).PossiblyNullContent(MenuWidget.ToSharedRef());
		GEngine->GameViewport->AddViewportWidgetContent(MenuContainer.ToSharedRef(), 100);
	}

	CurrentPage = Page;
	MenuWidget->ShowPage(Page);

	SetPause(true);
	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(MenuWidget);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);

	if (UCWContractSubsystem* Contract = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr)
	{
		Contract->LogEvent(TEXT("menu"), { { TEXT("page"), FString::FromInt(static_cast<int32>(Page)) } });
	}
}

void ACWPlayerController::CloseMenu()
{
	if (MenuContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuContainer.ToSharedRef());
	}
	MenuContainer.Reset();
	MenuWidget.Reset();
	CurrentPage = ECWMenuPage::None;

	SetPause(false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ACWPlayerController::MenuBack()
{
	switch (CurrentPage)
	{
	case ECWMenuPage::Settings:
		ShowMenu(PageBeforeSettings);
		break;
	case ECWMenuPage::Briefing:
		ShowMenu(ECWMenuPage::Main);
		break;
	case ECWMenuPage::Main:
		// El menu principal no se cierra con Esc: hay que elegir una opcion.
		break;
	default:
		CloseMenu();
		break;
	}
}

void ACWPlayerController::TogglePause()
{
	if (IsMenuOpen())
	{
		MenuBack();
	}
	else if (ShouldShowMenusOnStart() || FParse::Param(FCommandLine::Get(), TEXT("CWNoMenu")))
	{
		ShowMenu(ECWMenuPage::Pause);
	}
}

void ACWPlayerController::HandleContractStateChanged(ECWContractState Old, ECWContractState New)
{
	if ((New == ECWContractState::Completed || New == ECWContractState::Failed) && FApp::CanEverRender()
		&& !FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
	{
		// Un momento para leer el aviso antes de abrir la liquidacion.
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			ShowMenu(ECWMenuPage::Summary);
		}), 2.5f, false);
	}
}

// --- Acciones -----------------------------------------------------------------------

void ACWPlayerController::StartNewGame()
{
	ShowMenu(ECWMenuPage::Briefing);
}

void ACWPlayerController::AcceptContractFromBriefing()
{
	CloseMenu();
	if (UCWContractSubsystem* Contract = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr)
	{
		// Partida nueva: se descarta la anterior para que "Continuar" no mezcle estados.
		UCWContractSubsystem::DeleteSave(UCWContractSubsystem::DefaultSlot);
		if (Contract->GetState() != ECWContractState::Available)
		{
			Contract->ResetContract();
		}
		Contract->AcceptContract();
	}
}

void ACWPlayerController::AcceptContractKey()
{
	if (UCWContractSubsystem* Contract = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr)
	{
		if (Contract->GetState() == ECWContractState::Available)
		{
			Contract->AcceptContract();
		}
	}
}

void ACWPlayerController::ContinueGame()
{
	CloseMenu();
	QuickLoad();
}

void ACWPlayerController::QuickSave()
{
	UCWContractSubsystem* Contract = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr;
	if (!Contract)
	{
		return;
	}

	FString Error;
	if (Contract->SaveGameToSlot(UCWContractSubsystem::DefaultSlot, false, Error))
	{
		UCWNotificationSubsystem::Notify(this, TEXT("Partida guardada."), ECWNoticeSeverity::Success);
	}
	else
	{
		UCWNotificationSubsystem::Notify(this, FString::Printf(TEXT("No se puede guardar ahora: %s."), *Error), ECWNoticeSeverity::Warning);
	}
	if (CurrentPage == ECWMenuPage::Pause && MenuWidget.IsValid())
	{
		MenuWidget->ShowPage(ECWMenuPage::Pause);
	}
}

void ACWPlayerController::QuickLoad()
{
	UCWContractSubsystem* Contract = GetWorld() ? GetWorld()->GetSubsystem<UCWContractSubsystem>() : nullptr;
	if (!Contract)
	{
		return;
	}

	const bool bWasMenu = IsMenuOpen();
	FString Error;
	if (Contract->LoadGameFromSlot(UCWContractSubsystem::DefaultSlot, Error))
	{
		if (bWasMenu)
		{
			CloseMenu();
		}
		UCWNotificationSubsystem::Notify(this, TEXT("Partida cargada."), ECWNoticeSeverity::Success);
	}
	else
	{
		UCWNotificationSubsystem::Notify(this, FString::Printf(TEXT("No se pudo cargar: %s."), *Error), ECWNoticeSeverity::Error);
	}
}

void ACWPlayerController::RestartMission()
{
	// Reabrir el nivel restaura terreno y maquinas; ?cwnew acepta el contrato directamente.
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	CloseMenu();
	UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("cwnew"));
}

void ACWPlayerController::ReturnToMainMenu()
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	CloseMenu();
	UGameplayStatics::OpenLevel(this, FName(*Map), true);
}

void ACWPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

// --- Ajustes ------------------------------------------------------------------------

float ACWPlayerController::GetMouseSensitivity() const
{
	const IConsoleVariable* Var = FindCVar(TEXT("cw.Input.MouseSensitivity"));
	return Var ? Var->GetFloat() : 1.f;
}

void ACWPlayerController::SetMouseSensitivity(float Value)
{
	if (IConsoleVariable* Var = FindCVar(TEXT("cw.Input.MouseSensitivity")))
	{
		Var->Set(FMath::Clamp(Value, 0.1f, 5.f), ECVF_SetByGameSetting);
	}
	SaveSettings();
}

bool ACWPlayerController::GetInvertY() const
{
	const IConsoleVariable* Var = FindCVar(TEXT("cw.Input.InvertY"));
	return Var && Var->GetInt() != 0;
}

void ACWPlayerController::SetInvertY(bool bValue)
{
	if (IConsoleVariable* Var = FindCVar(TEXT("cw.Input.InvertY")))
	{
		Var->Set(bValue ? 1 : 0, ECVF_SetByGameSetting);
	}
	SaveSettings();
}

void ACWPlayerController::SetMasterVolume(float Value)
{
	MasterVolume = FMath::Clamp(Value, 0.f, 1.f);
	FApp::SetVolumeMultiplier(MasterVolume);
	SaveSettings();
}

bool ACWPlayerController::GetShowPerf() const
{
	const IConsoleVariable* Var = FindCVar(TEXT("cw.Perf.Hud"));
	return Var && Var->GetInt() != 0;
}

void ACWPlayerController::SetShowPerf(bool bValue)
{
	if (IConsoleVariable* Var = FindCVar(TEXT("cw.Perf.Hud")))
	{
		Var->Set(bValue ? 1 : 0, ECVF_SetByGameSetting);
	}
	SaveSettings();
}

bool ACWPlayerController::GetShowHints() const
{
	const IConsoleVariable* Var = FindCVar(TEXT("cw.Tutorial.Hints"));
	return !Var || Var->GetInt() != 0;
}

void ACWPlayerController::SetShowHints(bool bValue)
{
	if (IConsoleVariable* Var = FindCVar(TEXT("cw.Tutorial.Hints")))
	{
		Var->Set(bValue ? 1 : 0, ECVF_SetByGameSetting);
	}
	SaveSettings();
}

void ACWPlayerController::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}

	float Sensitivity = 0.f;
	if (GConfig->GetFloat(SettingsSection, TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni))
	{
		if (IConsoleVariable* Var = FindCVar(TEXT("cw.Input.MouseSensitivity"))) { Var->Set(Sensitivity, ECVF_SetByGameSetting); }
	}
	bool bFlag = false;
	if (GConfig->GetBool(SettingsSection, TEXT("InvertY"), bFlag, GGameUserSettingsIni))
	{
		if (IConsoleVariable* Var = FindCVar(TEXT("cw.Input.InvertY"))) { Var->Set(bFlag ? 1 : 0, ECVF_SetByGameSetting); }
	}
	if (GConfig->GetBool(SettingsSection, TEXT("ShowPerf"), bFlag, GGameUserSettingsIni))
	{
		if (IConsoleVariable* Var = FindCVar(TEXT("cw.Perf.Hud"))) { Var->Set(bFlag ? 1 : 0, ECVF_SetByGameSetting); }
	}
	if (GConfig->GetBool(SettingsSection, TEXT("ShowHints"), bFlag, GGameUserSettingsIni))
	{
		if (IConsoleVariable* Var = FindCVar(TEXT("cw.Tutorial.Hints"))) { Var->Set(bFlag ? 1 : 0, ECVF_SetByGameSetting); }
	}
	float Volume = 1.f;
	if (GConfig->GetFloat(SettingsSection, TEXT("MasterVolume"), Volume, GGameUserSettingsIni))
	{
		MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);
		FApp::SetVolumeMultiplier(MasterVolume);
	}
}

void ACWPlayerController::SaveSettings() const
{
	if (!GConfig)
	{
		return;
	}
	GConfig->SetFloat(SettingsSection, TEXT("MouseSensitivity"), GetMouseSensitivity(), GGameUserSettingsIni);
	GConfig->SetBool(SettingsSection, TEXT("InvertY"), GetInvertY(), GGameUserSettingsIni);
	GConfig->SetBool(SettingsSection, TEXT("ShowPerf"), GetShowPerf(), GGameUserSettingsIni);
	GConfig->SetBool(SettingsSection, TEXT("ShowHints"), GetShowHints(), GGameUserSettingsIni);
	GConfig->SetFloat(SettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
