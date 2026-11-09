#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWWeatherSystem.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyLight;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * Clima prototipo de H4 (F5.07): lluvia instanciada y humedad visual.
 *
 * La lluvia se dibuja con un unico UInstancedStaticMeshComponent (una sola llamada
 * de dibujo) alrededor de la camara. Al activarla tambien se oscurece el sol, se
 * enfria la luz y se espesa la niebla para simular un dia mojado.
 *
 * Uso en consola:
 *   cw.Weather.Rain 1     // activa lluvia + ambiente humedo
 *   cw.Weather.Rain 0     // vuelve al tiempo seco de tarde
 *   cw.Weather.Status     // muestra el estado actual
 */
UCLASS()
class CONSTRUCTIONWORLD_API ACWWeatherSystem : public AActor
{
	GENERATED_BODY()

public:
	ACWWeatherSystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "CW|Weather")
	void SetRainEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "CW|Weather")
	bool IsRainEnabled() const { return bRainEnabled; }

	/** Numero de gotas instanciadas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Weather")
	int32 MaxDrops = 700;

	/** Radio horizontal alrededor de la camara (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Weather")
	float RainRadiusCm = 2600.f;

	/** Altura del volumen de lluvia sobre la camara (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Weather")
	float RainHeightCm = 2800.f;

	/** Velocidad de caida (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Weather")
	float FallSpeedCmS = 3200.f;

	/** Inclinacion del viento (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CW|Weather")
	FVector WindCmS = FVector(120.f, 40.f, 0.f);

protected:
	void CaptureAtmosphere();
	void ApplyWetLook(bool bWet);
	void RebuildDrops();
	void UpdateDrops(float DeltaSeconds);
	void UpdateDropTransform(int32 Index);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CW|Weather")
	TObjectPtr<UInstancedStaticMeshComponent> RainInstances;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DropMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DropMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DropMID;

	TArray<FVector> DropOffsets;

	TWeakObjectPtr<ADirectionalLight> SunLight;
	TWeakObjectPtr<ASkyLight> SkyLightActor;
	TWeakObjectPtr<AExponentialHeightFog> HeightFog;

	float BaseSunIntensity = 8.f;
	float BaseSkyIntensity = 1.f;
	float BaseFogDensity = 0.02f;
	bool bAtmosphereCaptured = false;
	bool bRainEnabled = false;
};
