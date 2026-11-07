#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CWTerrainTypes.h"
#include "CWTerrainSubsystem.generated.h"

class ACWTerrainField;

/**
 * Punto unico de acceso al terreno deformable (H2).
 *
 * Localiza los ACWTerrainField del mundo y delega en ellos las operaciones de
 * excavacion, deposito y cambio de material. Tambien coordina el guardado.
 */
UCLASS()
class CONSTRUCTIONWORLD_API UCWTerrainSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	ACWTerrainField* FindTerrainAt(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	TArray<ACWTerrainField*> GetTerrainFields() const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	ACWTerrainField* GetPrimaryField() const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int32 GetFieldCount() const;

	/** Excava en el campo que contiene el punto. Devuelve m3 retirados. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	float DigAt(const FVector& WorldCenter, float RadiusCm, float MaxDepthCm,
		float RequestedVolumeM3, ECWSoilType& OutSoilType);

	/** Deposita en el campo que contiene el punto. Devuelve m3 aceptados. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	float DepositAt(const FVector& WorldCenter, float RadiusCm, float VolumeM3,
		ECWSoilType SoilType);

	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	bool SetSoilTypeInArea(const FVector& WorldCenter, float RadiusCm, ECWSoilType SoilType);

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetTotalVolumeM3() const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetTotalMassKg() const;

	/** Ruta por defecto del guardado de terreno. */
	UFUNCTION(BlueprintPure, Category = "CW|Terrain|Save")
	FString GetDefaultSavePath() const;

	UFUNCTION(BlueprintCallable, Category = "CW|Terrain|Save")
	bool SaveTerrain(const FString& FilePath, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "CW|Terrain|Save")
	bool LoadTerrain(const FString& FilePath, FString& OutError);
};
