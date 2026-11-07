#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWTerrainTypes.h"
#include "CWTerrainField.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Se emite cuando el terreno cambia (excavacion, deposito o cambio de material). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWTerrainChanged, FVector, Center, float, DeltaVolumeM3);

/**
 * Campo de altura deformable por chunks (H2 / Fase 4).
 *
 * - Malla procedural dividida en secciones (chunks) para reconstruir solo la zona afectada.
 * - Alturas por vertice en cm, material por celda y estado solido/suelto.
 * - Volumen calculado con el area real de cada celda y conservado entre excavacion y deposito.
 * - Colision por malla compleja; las reconstrucciones de colision se reparten en varios frames
 *   (MaxChunkRebuildsPerTick) para evitar picos (F3.09).
 * - Guardado/carga de deltas en binario con version, dimensiones y checksum (F3.12).
 *
 * Prototipo: sin RVT ni blending por altura; el color es un placeholder por material.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWTerrainField : public AActor
{
	GENERATED_BODY()

public:
	ACWTerrainField();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// --- Configuracion -------------------------------------------------------

	/** Numero de celdas en X (ancho). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain", meta = (ClampMin = "4", ClampMax = "256"))
	int32 GridCellsX = 128;

	/** Numero de celdas en Y (fondo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain", meta = (ClampMin = "4", ClampMax = "256"))
	int32 GridCellsY = 128;

	/** Tamano de celda en cm. 128 celdas x 117.1875 cm = 150 m. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain", meta = (ClampMin = "50.0"))
	float CellSizeCm = 117.1875f;

	/** Altura de referencia (nivel del suelo original) en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	float ReferenceHeightCm = 0.f;

	/** Altura minima permitida al excavar (cm, relativa al actor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	float MinHeightCm = -600.f;

	/** Altura maxima permitida al depositar (cm, relativa al actor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	float MaxHeightCm = 400.f;

	/** Material por defecto del terreno. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	ECWSoilType DefaultSoilType = ECWSoilType::Earth;

	/** Celdas por lado de cada chunk de malla/colision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain", meta = (ClampMin = "2", ClampMax = "64"))
	int32 ChunkCells = 8;

	/** Maximo de chunks cuya colision se reconstruye por frame (F3.09). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxChunkRebuildsPerTick = 4;

	/** Genera colision compleja para el prototipo (F3.02). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	bool bGenerateCollision = true;

	/** Excava una zanja inicial para la escena comparativa (F3.01). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	bool bCarveInitialPit = false;

	/** Profundidad de la zanja inicial en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	float InitialPitDepthCm = 150.f;

	/** Material visual del terreno (placeholder). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterialAsset;

	// --- Consultas -----------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetHeightAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	FVector GetCellCenter(int32 CellX, int32 CellY) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	FVector GetFieldCenter() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	FVector2D GetFieldSizeCm() const
	{
		return FVector2D(GridCellsX * CellSizeCm, GridCellsY * CellSizeCm);
	}

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	bool ContainsLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetTotalVolumeM3() const { return TotalVolumeM3; }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetVolumeM3InArea(const FVector& WorldCenter, float RadiusCm) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetMassKgInArea(const FVector& WorldCenter, float RadiusCm) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	float GetTotalMassKg() const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	ECWSoilType GetSoilTypeAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int32 GetVertexCount() const { return Heights.Num(); }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int32 GetCellCount() const { return GridCellsX * GridCellsY; }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int32 GetPendingChunkCount() const { return DirtyChunks.Num(); }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int32 GetCollisionRebuildsLastTick() const { return CollisionRebuildsLastTick; }

	UFUNCTION(BlueprintPure, Category = "CW|Terrain")
	int64 GetApproxMemoryBytes() const;

	// --- Operaciones ---------------------------------------------------------

	/**
	 * Excava un cono suave centrado en WorldCenter.
	 * Devuelve el volumen realmente retirado en m3 (puede ser menor que RequestedVolumeM3
	 * si se alcanza MaxDepthCm, MinHeightCm o el limite del campo).
	 */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	float DigAt(const FVector& WorldCenter, float RadiusCm, float MaxDepthCm, float RequestedVolumeM3);

	/**
	 * Deposita material en un monton suave. Devuelve el volumen realmente aceptado en m3.
	 */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	float DepositAt(const FVector& WorldCenter, float RadiusCm, float VolumeM3, ECWSoilType SoilType);

	/** Cambia el tipo de suelo de las celdas dentro del radio. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	bool SetSoilTypeInArea(const FVector& WorldCenter, float RadiusCm, ECWSoilType SoilType);

	/** Reconstruye toda la malla. Si bWithCollision, tambien la colision. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	void RebuildAll(bool bWithCollision = true);

	/** Procesa la cola de chunks pendientes; 0 = todos. Devuelve cuantos proceso. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	int32 FlushDirtyChunks(int32 MaxChunks = 0);

	/** Excava la zanja inicial de la escena comparativa. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain")
	void CarveInitialPit();

	// --- Persistencia (F3.12) ------------------------------------------------

	/** Guarda el estado en un archivo binario atomico (tmp + rename). */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain|Save")
	bool SaveStateToFile(const FString& FilePath, FString& OutError);

	/** Carga el estado desde archivo; valida version, dimensiones y checksum. */
	UFUNCTION(BlueprintCallable, Category = "CW|Terrain|Save")
	bool LoadStateFromFile(const FString& FilePath, FString& OutError);

	/** Serializa el estado (para pruebas y guardado global). */
	bool SerializeState(FArchive& Ar, FString* OutError = nullptr);

	UPROPERTY(BlueprintAssignable, Category = "CW|Terrain")
	FCWTerrainChanged OnTerrainChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Terrain")
	TObjectPtr<UProceduralMeshComponent> TerrainMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TerrainMaterial;

	void InitializeGrid();
	void ApplyInitialPit();
	void BuildSection(int32 ChunkX, int32 ChunkY, bool bWithCollision);
	void MarkChunkDirty(int32 ChunkX, int32 ChunkY);
	void MarkAreaDirty(const FVector& WorldCenter, float RadiusCm);
	void ProcessDirtyChunks();
	void UpdateMaterialColor();

	int32 VertexIndex(int32 X, int32 Y) const { return Y * (GridCellsX + 1) + X; }
	int32 ChunkIndex(int32 ChunkX, int32 ChunkY) const { return ChunkY * NumChunksX + ChunkX; }
	FVector VertexLocalPosition(int32 X, int32 Y) const;
	bool WorldToGrid(const FVector& WorldLocation, float& OutGridX, float& OutGridY) const;
	float Falloff(float NormalizedDistance) const;
	float ComputeCellVolumeM3(int32 CellX, int32 CellY) const;
	float ComputeAreaVolumeM3(const FVector& WorldCenter, float RadiusCm) const;
	int32 ClampCellX(int32 X) const { return FMath::Clamp(X, 0, GridCellsX - 1); }
	int32 ClampCellY(int32 Y) const { return FMath::Clamp(Y, 0, GridCellsY - 1); }

	/** Alturas por vertice, en cm, relativas al actor. */
	TArray<float> Heights;

	/** Material por celda. */
	TArray<ECWSoilType> Materials;

	/** 0 = solido, 1 = suelto. */
	TArray<uint8> CellStates;

	TSet<int32> DirtyChunks;
	int32 NumChunksX = 0;
	int32 NumChunksY = 0;
	float TotalVolumeM3 = 0.f;
	int32 CollisionRebuildsLastTick = 0;
	bool bGridReady = false;
};
