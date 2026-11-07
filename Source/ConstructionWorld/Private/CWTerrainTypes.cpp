#include "CWTerrainTypes.h"

namespace CWSoil
{
	FCWSoilProperties GetSoilProperties(ECWSoilType Type)
	{
		FCWSoilProperties Props;
		Props.Type = Type;

		switch (Type)
		{
		case ECWSoilType::Gravel:
			Props.DisplayName = TEXT("Grava");
			Props.DensityKgPerCubicMeter = 1800.f;
			Props.AngleOfReposeDegrees = 40.f;
			Props.Color = FLinearColor(0.42f, 0.40f, 0.37f);
			break;

		case ECWSoilType::Sand:
			Props.DisplayName = TEXT("Arena");
			Props.DensityKgPerCubicMeter = 1500.f;
			Props.AngleOfReposeDegrees = 30.f;
			Props.Color = FLinearColor(0.72f, 0.62f, 0.40f);
			break;

		case ECWSoilType::Clay:
			Props.DisplayName = TEXT("Arcilla");
			Props.DensityKgPerCubicMeter = 1900.f;
			Props.AngleOfReposeDegrees = 45.f;
			Props.Color = FLinearColor(0.38f, 0.25f, 0.20f);
			break;

		case ECWSoilType::Earth:
		default:
			Props.DisplayName = TEXT("Tierra");
			Props.DensityKgPerCubicMeter = 1600.f;
			Props.AngleOfReposeDegrees = 35.f;
			Props.Color = FLinearColor(0.34f, 0.24f, 0.14f);
			break;
		}

		return Props;
	}

	float GetDensityKgPerCubicMeter(ECWSoilType Type)
	{
		return GetSoilProperties(Type).DensityKgPerCubicMeter;
	}
}
