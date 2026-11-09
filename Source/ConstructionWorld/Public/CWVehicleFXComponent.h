#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CWVehicleFXComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** Estado de una bocanada de polvo del pool. */
struct FCWDustPuffState
{
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float Life = 1.4f;
	float StartScale = 0.2f;
	float EndScale = 1.4f;
	bool bActive = false;
};

/**
 * Efectos visuales baratos para maquinaria (F5.04).
 *
 * - Polvo: pool de esferas translucidas que crecen, se elevan y se desvanecen.
 * - Huellas: pool de planos oscuros que se colocan bajo las ruedas/orugas con un
 *   trazado de suelo para no flotar ni atravesar el terreno.
 *
 * Se crea por defecto en ACWVehicleBase, por lo que cualquier vehiculo lo hereda.
 * No necesita Niagara ni assets de particulas: solo el material translucido
 * /Game/ConstructionWorld/Materials/M_CW_TranslucentFX y las mallas basicas.
 */
UCLASS(ClassGroup = (CW), meta = (BlueprintSpawnableComponent))
class CONSTRUCTIONWORLD_API UCWVehicleFXComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCWVehicleFXComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Emite polvo en una posicion del mundo. */
	UFUNCTION(BlueprintCallable, Category = "CW|FX")
	void EmitDust(const FVector& WorldLocation, const FLinearColor& Color,
		int32 PuffCount = 8, float Strength = 1.f);

	/** Activa o desactiva las huellas de ruedas/orugas. */
	UFUNCTION(BlueprintCallable, Category = "CW|FX")
	void SetTrackMarksEnabled(bool bEnabled) { bTrackMarksEnabled = bEnabled; }

	/** Distancia recorrida entre pares de huellas (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|FX|TrackMarks")
	float TrackMarkSpacingCm = 70.f;

	/** Numero de planos de huella en el pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|FX|TrackMarks")
	int32 MaxTrackMarks = 48;

	/** Segundos que una huella permanece visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|FX|TrackMarks")
	float TrackMarkLifeSeconds = 26.f;

	/** Numero de esferas de polvo en el pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|FX|Dust")
	int32 MaxDustPuffs = 32;

protected:
	void CreatePools();
	void UpdateDust(float DeltaTime);
	void UpdateTrackMarks(float DeltaTime, const FVector& OwnerLocation);
	bool FindGroundZ(const FVector& WorldLocation, float& OutGroundZ) const;
	void PlaceTrackMark(int32 Index, const FVector& WorldLocation, const FRotator& WorldRotation,
		float HalfWidthCm, float HalfLengthCm);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PlaneMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FXMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> DustMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DustMIDs;

	TArray<FCWDustPuffState> DustStates;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> TrackMarkMeshes;

	TArray<float> TrackMarkAges;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TrackMarkMID;

	bool bPoolsCreated = false;
	bool bTrackMarksEnabled = true;
	FVector LastTrackMarkLocation = FVector::ZeroVector;
	bool bHasLastTrackMarkLocation = false;
};
