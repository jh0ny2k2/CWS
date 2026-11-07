#pragma once

#include "CoreMinimal.h"

/**
 * Convenciones de unidades del proyecto (F0.06).
 *
 * Documento completo: docs/CONVENCIONES.md
 *
 * Resumen:
 *  - 1 Unreal Unit (UU) = 1 centimetro (cm).
 *  - Longitud y posicion: centimetros (FVector, distancias, tamanos).
 *  - Masa: kilogramos (kg) en gameplay; el motor usa kg para fisica.
 *  - Volumen de material: metros cubicos (m3) en contratos/economia, litros en UI.
 *  - Velocidad de diseno: km/h; velocidad interna del motor: cm/s.
 *  - Angulos: grados en API/editor; radianes solo en formulas trigonometricas.
 *  - Tiempo: segundos (float), milisegundos solo para metricas de frame.
 *  - Ejes: X adelante, Y derecha, Z arriba (mundo izquierdo, Z-up).
 */
namespace CWUnits
{
	/** Centimetros por metro. */
	static constexpr float CmPerMeter = 100.f;

	/** Centimetros por kilometro. */
	static constexpr float CmPerKilometer = 100000.f;

	/** Segundos por hora. */
	static constexpr float SecondsPerHour = 3600.f;

	/** Kilogramos por tonelada metrica. */
	static constexpr float KgPerTonne = 1000.f;

	/** Litros por metro cubico. */
	static constexpr float LitersPerCubicMeter = 1000.f;

	/** Centimetros cubicos por metro cubico. */
	static constexpr double CubicCmPerCubicMeter = 1000000.0;

	/** Convierte metros a Unreal Units (cm). */
	FORCEINLINE float MetersToUU(const float Meters)
	{
		return Meters * CmPerMeter;
	}

	/** Convierte Unreal Units (cm) a metros. */
	FORCEINLINE float UUToMeters(const float UU)
	{
		return UU / CmPerMeter;
	}

	/** Convierte km/h a cm/s (unidad interna del motor). */
	FORCEINLINE float KmhToCms(const float Kmh)
	{
		return Kmh * (CmPerKilometer / SecondsPerHour);
	}

	/** Convierte cm/s a km/h. */
	FORCEINLINE float CmsToKmh(const float Cms)
	{
		return Cms * (SecondsPerHour / CmPerKilometer);
	}

	/** Convierte toneladas a kilogramos. */
	FORCEINLINE float TonnesToKg(const float Tonnes)
	{
		return Tonnes * KgPerTonne;
	}

	/** Convierte kilogramos a toneladas. */
	FORCEINLINE float KgToTonnes(const float Kg)
	{
		return Kg / KgPerTonne;
	}

	/** Convierte metros cubicos a litros. */
	FORCEINLINE float CubicMetersToLiters(const float CubicMeters)
	{
		return CubicMeters * LitersPerCubicMeter;
	}

	/** Convierte litros a metros cubicos. */
	FORCEINLINE float LitersToCubicMeters(const float Liters)
	{
		return Liters / LitersPerCubicMeter;
	}

	/** Convierte un volumen en cm3 (fisica) a m3 (contratos/economia). */
	FORCEINLINE double CubicCmToCubicMeters(const double CubicCm)
	{
		return CubicCm / CubicCmPerCubicMeter;
	}

	/** Convierte grados a radianes. */
	FORCEINLINE float DegreesToRadians(const float Degrees)
	{
		return FMath::DegreesToRadians(Degrees);
	}

	/** Convierte radianes a grados. */
	FORCEINLINE float RadiansToDegrees(const float Radians)
	{
		return FMath::RadiansToDegrees(Radians);
	}

	/** Formatea una velocidad interna (cm/s) como texto en km/h. */
	FORCEINLINE FString FormatSpeedKmh(const float Cms)
	{
		return FString::Printf(TEXT("%.1f km/h"), CmsToKmh(Cms));
	}

	/** Formatea un volumen interno (cm3) como texto en m3. */
	FORCEINLINE FString FormatVolumeM3(const double CubicCm)
	{
		return FString::Printf(TEXT("%.2f m3"), CubicCmToCubicMeters(CubicCm));
	}
}
