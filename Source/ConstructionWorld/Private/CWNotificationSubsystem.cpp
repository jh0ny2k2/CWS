#include "CWNotificationSubsystem.h"

#include "CWLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UCWNotificationSubsystem::Notify(const UObject* WorldContext, const FString& Text,
	ECWNoticeSeverity Severity)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (UCWNotificationSubsystem* Subsystem = World ? World->GetSubsystem<UCWNotificationSubsystem>() : nullptr)
	{
		Subsystem->Push(Text, Severity);
	}
}

double UCWNotificationSubsystem::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

void UCWNotificationSubsystem::Push(const FString& Text, ECWNoticeSeverity Severity)
{
	if (Text.IsEmpty())
	{
		return;
	}

	const double Now = GetNow();

	// Mismo aviso repetido en poco tiempo: se agrupa para no inundar la pantalla.
	if (History.Num() > 0)
	{
		FCWNotice& Last = History.Last();
		if (Last.Text == Text && Now - Last.TimeSeconds < 4.0)
		{
			Last.TimeSeconds = Now;
			++Last.Count;
			return;
		}
	}

	FCWNotice Notice;
	Notice.Text = Text;
	Notice.Severity = Severity;
	Notice.TimeSeconds = Now;
	History.Add(Notice);
	if (History.Num() > 64)
	{
		History.RemoveAt(0, History.Num() - 64);
	}

	UE_LOG(LogCWContract, Log, TEXT("Aviso[%d]: %s"), static_cast<int32>(Severity), *Text);
}

TArray<FCWNotice> UCWNotificationSubsystem::GetActiveNotices(double LifetimeSeconds) const
{
	TArray<FCWNotice> Active;
	const double Now = GetNow();
	for (int32 Index = FMath::Max(0, History.Num() - 4); Index < History.Num(); ++Index)
	{
		if (Now - History[Index].TimeSeconds <= LifetimeSeconds)
		{
			Active.Add(History[Index]);
		}
	}
	return Active;
}
