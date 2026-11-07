#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWTerrainTypes.h"
#include "CWDumpZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ACWStockpile;

/** Evento emitido cuando un camion descarga material en la zona. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWLoadDelivered, float, CubicMeters, AActor*, Source);

/**
 * Zona de descarga (F2.13).
 *
 * Volumen de trigger donde el camion volquete puede descargar. Acumula el
 * material recibido, emite OnLoadDelivered y registra cada entrega en el log.
 * Se coloca en el nivel como cualquier actor; su caja define el area valida.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWDumpZone : public AActor
{
	GENERATED_BODY()

public:
	ACWDumpZone();

	virtual void BeginPlay() override;

	/** Recibe material de un camion. Devuelve true si se acepto algo. */
	UFUNCTION(BlueprintCallable, Category = "CW|DumpZone")
	bool DeliverLoad(float CubicMeters, AActor* Source);

	/** Comprueba si un actor esta dentro del volumen de la zona. */
	UFUNCTION(BlueprintPure, Category = "CW|DumpZone")
	bool IsActorInside(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "CW|DumpZone")
	float GetDeliveredCubicMeters() const { return DeliveredCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpZone")
	float GetCapacityCubicMeters() const { return CapacityCubicMeters; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpZone")
	float GetDepositedIntoTerrainM3() const { return DepositedIntoTerrainM3; }

	UFUNCTION(BlueprintPure, Category = "CW|DumpZone")
	FText GetZoneStatusText() const;

	UPROPERTY(BlueprintAssignable, Category = "CW|DumpZone")
	FCWLoadDelivered OnLoadDelivered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	FString ZoneName = TEXT("Zona de descarga");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	float CapacityCubicMeters = 200.f;

	/** Si es true, el material entregado se deposita en el terreno (H2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	bool bDepositIntoTerrain = true;

	/** Radio del monton depositado en el terreno (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	float DepositRadiusCm = 500.f;

	/** Material que se deposita en el terreno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	ECWSoilType DepositSoilType = ECWSoilType::Earth;

	/** Acopio visual opcional; si esta asignado, recibe el material en lugar del terreno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|DumpZone")
	TObjectPtr<ACWStockpile> LinkedStockpile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpZone")
	TObjectPtr<UBoxComponent> ZoneBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpZone")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpZone")
	float DeliveredCubicMeters = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|DumpZone")
	float DepositedIntoTerrainM3 = 0.f;
};
