#include "CWTerrainSubsystem.h"

#include "CWLog.h"
#include "CWTerrainField.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"

void UCWTerrainSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const TArray<ACWTerrainField*> Fields = GetTerrainFields();
	UE_LOG(LogCWTerrain, Log, TEXT("CWTerrainSubsystem: %d campo(s) de terreno en el mundo."),
		Fields.Num());
}

TArray<ACWTerrainField*> UCWTerrainSubsystem::GetTerrainFields() const
{
	TArray<ACWTerrainField*> Fields;
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<ACWTerrainField> It(World); It; ++It)
		{
			if (ACWTerrainField* Field = *It)
			{
				Fields.Add(Field);
			}
		}
	}

	return Fields;
}

ACWTerrainField* UCWTerrainSubsystem::FindTerrainAt(const FVector& WorldLocation) const
{
	for (ACWTerrainField* Field : GetTerrainFields())
	{
		if (Field && Field->ContainsLocation(WorldLocation))
		{
			return Field;
		}
	}

	return nullptr;
}

ACWTerrainField* UCWTerrainSubsystem::GetPrimaryField() const
{
	const TArray<ACWTerrainField*> Fields = GetTerrainFields();
	return Fields.Num() > 0 ? Fields[0] : nullptr;
}

int32 UCWTerrainSubsystem::GetFieldCount() const
{
	return GetTerrainFields().Num();
}

float UCWTerrainSubsystem::DigAt(const FVector& WorldCenter, float RadiusCm,
	float MaxDepthCm, float RequestedVolumeM3, ECWSoilType& OutSoilType)
{
	ACWTerrainField* Field = FindTerrainAt(WorldCenter);
	if (!Field)
	{
		OutSoilType = ECWSoilType::Earth;
		return 0.f;
	}

	OutSoilType = Field->GetSoilTypeAtLocation(WorldCenter);
	return Field->DigAt(WorldCenter, RadiusCm, MaxDepthCm, RequestedVolumeM3);
}

float UCWTerrainSubsystem::DepositAt(const FVector& WorldCenter, float RadiusCm,
	float VolumeM3, ECWSoilType SoilType)
{
	ACWTerrainField* Field = FindTerrainAt(WorldCenter);
	return Field ? Field->DepositAt(WorldCenter, RadiusCm, VolumeM3, SoilType) : 0.f;
}

bool UCWTerrainSubsystem::SetSoilTypeInArea(const FVector& WorldCenter,
	float RadiusCm, ECWSoilType SoilType)
{
	ACWTerrainField* Field = FindTerrainAt(WorldCenter);
	return Field ? Field->SetSoilTypeInArea(WorldCenter, RadiusCm, SoilType) : false;
}

float UCWTerrainSubsystem::GetTotalVolumeM3() const
{
	float Total = 0.f;
	for (const ACWTerrainField* Field : GetTerrainFields())
	{
		if (Field)
		{
			Total += Field->GetTotalVolumeM3();
		}
	}

	return Total;
}

float UCWTerrainSubsystem::GetTotalMassKg() const
{
	float Total = 0.f;
	for (const ACWTerrainField* Field : GetTerrainFields())
	{
		if (Field)
		{
			Total += Field->GetTotalMassKg();
		}
	}

	return Total;
}

FString UCWTerrainSubsystem::GetDefaultSavePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Terrain"), TEXT("CW_Terrain.cwt"));
}

bool UCWTerrainSubsystem::SaveTerrain(const FString& FilePath, FString& OutError)
{
	ACWTerrainField* Field = GetPrimaryField();
	if (!Field)
	{
		OutError = TEXT("no hay campo de terreno en el mundo");
		return false;
	}

	return Field->SaveStateToFile(FilePath, OutError);
}

bool UCWTerrainSubsystem::LoadTerrain(const FString& FilePath, FString& OutError)
{
	ACWTerrainField* Field = GetPrimaryField();
	if (!Field)
	{
		OutError = TEXT("no hay campo de terreno en el mundo");
		return false;
	}

	return Field->LoadStateFromFile(FilePath, OutError);
}
