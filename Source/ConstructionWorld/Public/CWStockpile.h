#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWTerrainTypes.h"
#include "CWStockpile.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Acopio de material (H2 / F3.08).
 *
 * Monton conico cuyo tamano representa el volumen almacenado. Sirve para
 * visualizar el material excavado y para depositos temporales. El volumen es
 * abstracto (m3) pero se convierte a masa con la densidad del tipo de suelo.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWStockpile : public AActor
{
	GENERATED_BODY()

public:
	ACWStockpile();

	virtual void BeginPlay() override;

	/** Anade material. Devuelve los m3 aceptados (limitado por MaxVolumeM3). */
	UFUNCTION(BlueprintCallable, Category = "CW|Stockpile")
	float AddMaterial(float CubicMeters);

	/** Retira material. Devuelve los m3 realmente retirados. */
	UFUNCTION(BlueprintCallable, Category = "CW|Stockpile")
	float RemoveMaterial(float CubicMeters);

	UFUNCTION(BlueprintCallable, Category = "CW|Stockpile")
	void SetSoilType(ECWSoilType NewSoilType);

	UFUNCTION(BlueprintPure, Category = "CW|Stockpile")
	float GetVolumeM3() const { return VolumeM3; }

	UFUNCTION(BlueprintPure, Category = "CW|Stockpile")
	float GetMassKg() const;

	UFUNCTION(BlueprintPure, Category = "CW|Stockpile")
	ECWSoilType GetSoilType() const { return SoilType; }

	UFUNCTION(BlueprintPure, Category = "CW|Stockpile")
	FVector GetPileCenter() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Stockpile")
	float MaxVolumeM3 = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Stockpile")
	float BaseRadiusCm = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Stockpile")
	float MaxHeightCm = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Stockpile")
	ECWSoilType SoilType = ECWSoilType::Earth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Stockpile")
	TObjectPtr<UMaterialInterface> PileMaterialAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Stockpile")
	TObjectPtr<UStaticMeshComponent> PileMesh;

protected:
	void UpdateVisual();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Stockpile")
	float VolumeM3 = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PileMaterial;
};
