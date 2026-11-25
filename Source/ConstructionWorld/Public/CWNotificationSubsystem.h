#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWNotificationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECWNoticeSeverity : uint8
{
	Info,
	Success,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct FCWNotice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CW|Notice")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "CW|Notice")
	ECWNoticeSeverity Severity = ECWNoticeSeverity::Info;

	/** Segundos de mundo (tiempo real, no se para en pausa) en que se emitio. */
	double TimeSeconds = 0.0;

	/** Veces que se ha repetido seguido (se muestra "x3" en vez de apilar). */
	int32 Count = 1;
};

/**
 * Avisos cortos para el jugador (F4.04): errores comprensibles, pasos del tutorial
 * completados y eventos del contrato. El HUD dibuja los ultimos avisos con fundido.
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWNotificationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Atajo seguro: no hace nada si no hay mundo (p. ej. objetos de clase por defecto). */
	static void Notify(const UObject* WorldContext, const FString& Text,
		ECWNoticeSeverity Severity = ECWNoticeSeverity::Info);

	void Push(const FString& Text, ECWNoticeSeverity Severity);

	/** Avisos que aun deben mostrarse (los mas nuevos al final). */
	TArray<FCWNotice> GetActiveNotices(double LifetimeSeconds = 6.0) const;

	const TArray<FCWNotice>& GetHistory() const { return History; }

	double GetNow() const;

private:
	TArray<FCWNotice> History;
};
