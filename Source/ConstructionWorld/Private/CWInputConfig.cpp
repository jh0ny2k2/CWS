#include "CWInputConfig.h"

#include "Engine/World.h"
#include "EnhancedActionKeyMapping.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

static TAutoConsoleVariable<float> CVarCWMouseSensitivity(
	TEXT("cw.Input.MouseSensitivity"),
	1.f,
	TEXT("Multiplicador de sensibilidad del raton para la camara."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarCWGamepadLookRate(
	TEXT("cw.Input.GamepadLookRate"),
	45.f,
	TEXT("Velocidad de giro de camara con el stick derecho (unidades por segundo)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarCWInvertY(
	TEXT("cw.Input.InvertY"),
	0,
	TEXT("Invierte el eje vertical de la camara (1 = invertido)."),
	ECVF_Default);

namespace CWInput
{
	float GetMouseSensitivity()
	{
		return FMath::Clamp(CVarCWMouseSensitivity.GetValueOnGameThread(), 0.05f, 10.f);
	}

	float GetGamepadLookRate()
	{
		return FMath::Clamp(CVarCWGamepadLookRate.GetValueOnGameThread(), 1.f, 400.f);
	}

	bool IsLookYInverted()
	{
		return CVarCWInvertY.GetValueOnGameThread() != 0;
	}

	void ApplyLook(APawn* Pawn, const FVector2D& Axis, bool bIsRate)
	{
		if (!Pawn || !Pawn->GetController())
		{
			return;
		}

		float Scale = GetMouseSensitivity();
		if (bIsRate)
		{
			const UWorld* World = Pawn->GetWorld();
			Scale = GetGamepadLookRate() * (World ? World->GetDeltaSeconds() : 0.f);
		}

		const float PitchSign = IsLookYInverted() ? -1.f : 1.f;
		Pawn->AddControllerYawInput(Axis.X * Scale);
		Pawn->AddControllerPitchInput(Axis.Y * Scale * PitchSign);
	}

	FEnhancedActionKeyMapping& MapKey(
		UInputMappingContext* Context, UInputAction* Action, const FKey& Key, UObject* Outer,
		bool bNegate, bool bDeadZone)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bDeadZone)
		{
			UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(Outer);
			DeadZone->LowerThreshold = 0.2f;
			Mapping.Modifiers.Add(DeadZone);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Outer));
		}
		return Mapping;
	}
}
