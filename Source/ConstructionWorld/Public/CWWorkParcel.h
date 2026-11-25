#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWWorkParcel.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Parcela de trabajo de un contrato (F4.02).
 *
 * Define en planta (XY) el area donde la excavacion cuenta para el contrato.
 * Se baliza con estacas, cinta entre ellas y un cartel con el codigo del
 * contrato. La altura no limita: se excava hacia abajo.
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWWorkParcel : public AActor
{
	GENERATED_BODY()

public:
	ACWWorkParcel();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Medio tamano de la parcela en planta (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Parcel", meta = (ClampMin = "100.0"))
	FVector2D HalfSizeCm = FVector2D(450.f, 450.f);

	/** Codigo del contrato que usa esta parcela. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Parcel")
	FString ContractId = TEXT("CWS-001");

	/** Margen fuera del borde que aun se acepta (cuchara que roza la cinta), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Parcel", meta = (ClampMin = "0.0"))
	float EdgeToleranceCm = 40.f;

	UFUNCTION(BlueprintPure, Category = "CW|Parcel")
	bool ContainsLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "CW|Parcel")
	float GetAreaM2() const { return (HalfSizeCm.X * 2.f / 100.f) * (HalfSizeCm.Y * 2.f / 100.f); }

	/** Coloca estacas, cinta y cartel segun HalfSizeCm y el suelo bajo cada estaca. */
	UFUNCTION(BlueprintCallable, Category = "CW|Parcel")
	void RebuildMarkers();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Parcel")
	TObjectPtr<UBoxComponent> AreaBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Parcel")
	TObjectPtr<UTextRenderComponent> SignText;

private:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Stakes;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Tapes;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SignBoard;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SignPost;
};
