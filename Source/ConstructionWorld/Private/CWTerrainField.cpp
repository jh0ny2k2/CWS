#include "CWTerrainField.h"

#include "CWLog.h"
#include "ProceduralMeshComponent.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr uint32 CWTerrainMagic = 0x43575431; // "CWT1"
	constexpr uint32 CWTerrainVersion = 1;

	struct FCWTerrainStateHeader
	{
		uint32 Magic = CWTerrainMagic;
		uint32 Version = CWTerrainVersion;
		int32 CellsX = 0;
		int32 CellsY = 0;
		float CellSizeCm = 0.f;
		float ReferenceHeightCm = 0.f;
		float MinHeightCm = 0.f;
		float MaxHeightCm = 0.f;
		int32 HeightCount = 0;
		int32 MaterialCount = 0;
		uint32 Checksum = 0;
	};

	uint32 ComputeTerrainChecksum(const TArray<float>& HeightData,
		const TArray<uint8>& Materials, const TArray<uint8>& States)
	{
		uint32 Checksum = FCrc::MemCrc32(
			HeightData.GetData(), HeightData.Num() * sizeof(float), 0);
		Checksum = FCrc::MemCrc32(Materials.GetData(), Materials.Num(), Checksum);
		Checksum = FCrc::MemCrc32(States.GetData(), States.Num(), Checksum);
		return Checksum;
	}
}

ACWTerrainField::ACWTerrainField()
{
	PrimaryActorTick.bCanEverTick = true;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	SetRootComponent(TerrainMesh);
	TerrainMesh->bUseComplexAsSimpleCollision = true;
	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TerrainMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TerrainMesh->SetGenerateOverlapEvents(false);
	TerrainMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		TerrainMaterialAsset = DefaultMaterial.Object;
	}
}

void ACWTerrainField::BeginPlay()
{
	Super::BeginPlay();

	InitializeGrid();
	ApplyInitialPit();

	if (TerrainMaterialAsset)
	{
		TerrainMaterial = UMaterialInstanceDynamic::Create(TerrainMaterialAsset, this);
		if (TerrainMaterial)
		{
			TerrainMesh->SetMaterial(0, TerrainMaterial);
			UpdateMaterialColor();
		}
	}
	else
	{
		TerrainMesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
	}

	RebuildAll(true);

	UE_LOG(LogCWTerrain, Log,
		TEXT("CWTerrainField '%s': %dx%d celdas (%.1f cm), %d chunks, volumen %.2f m3, ~%lld bytes."),
		*GetName(), GridCellsX, GridCellsY, CellSizeCm, NumChunksX * NumChunksY,
		TotalVolumeM3, GetApproxMemoryBytes());
}

void ACWTerrainField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CollisionRebuildsLastTick = 0;
	ProcessDirtyChunks();
}

void ACWTerrainField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// La malla procedural no se serializa en el mapa: se regenera al cargar o al
	// cambiar propiedades en el editor. En BeginPlay se vuelve a inicializar.
	InitializeGrid();
	ApplyInitialPit();
	RebuildAll(true);
}

void ACWTerrainField::InitializeGrid()
{
	GridCellsX = FMath::Clamp(GridCellsX, 4, 256);
	GridCellsY = FMath::Clamp(GridCellsY, 4, 256);
	CellSizeCm = FMath::Max(50.f, CellSizeCm);
	ChunkCells = FMath::Clamp(ChunkCells, 2, 64);
	MaxChunkRebuildsPerTick = FMath::Clamp(MaxChunkRebuildsPerTick, 1, 64);

	NumChunksX = FMath::DivideAndRoundUp(GridCellsX, ChunkCells);
	NumChunksY = FMath::DivideAndRoundUp(GridCellsY, ChunkCells);

	Heights.SetNum((GridCellsX + 1) * (GridCellsY + 1));
	for (float& Height : Heights)
	{
		Height = ReferenceHeightCm;
	}

	Materials.SetNum(GridCellsX * GridCellsY);
	for (ECWSoilType& Material : Materials)
	{
		Material = DefaultSoilType;
	}

	CellStates.SetNumZeroed(GridCellsX * GridCellsY);
	DirtyChunks.Reset();
	TerrainMesh->ClearAllMeshSections();

	TotalVolumeM3 = 0.f;
	bGridReady = true;
}

void ACWTerrainField::ApplyInitialPit()
{
	if (!bCarveInitialPit || !bGridReady)
	{
		return;
	}

	const float CenterX = GridCellsX * 0.5f;
	const float CenterY = GridCellsY * 0.5f;
	const float RadiusX = FMath::Max(1.f, GridCellsX * 0.25f);
	const float RadiusY = FMath::Max(1.f, GridCellsY * 0.25f);

	for (int32 Y = 0; Y <= GridCellsY; ++Y)
	{
		for (int32 X = 0; X <= GridCellsX; ++X)
		{
			const float DX = FMath::Abs(X - CenterX) / RadiusX;
			const float DY = FMath::Abs(Y - CenterY) / RadiusY;
			if (DX >= 1.f || DY >= 1.f)
			{
				continue;
			}

			const float Fade = (1.f - DX * DX) * (1.f - DY * DY);
			Heights[VertexIndex(X, Y)] -= InitialPitDepthCm * Fade;
		}
	}

	TotalVolumeM3 = 0.f;
	for (int32 Y = 0; Y < GridCellsY; ++Y)
	{
		for (int32 X = 0; X < GridCellsX; ++X)
		{
			TotalVolumeM3 += ComputeCellVolumeM3(X, Y);
		}
	}

	for (int32 CY = 0; CY < NumChunksY; ++CY)
	{
		for (int32 CX = 0; CX < NumChunksX; ++CX)
		{
			MarkChunkDirty(CX, CY);
		}
	}
}

FVector ACWTerrainField::VertexLocalPosition(int32 X, int32 Y) const
{
	const float HalfWidth = GridCellsX * CellSizeCm * 0.5f;
	const float HalfHeight = GridCellsY * CellSizeCm * 0.5f;
	const float Z = Heights.IsValidIndex(VertexIndex(X, Y)) ? Heights[VertexIndex(X, Y)] : ReferenceHeightCm;
	return FVector(X * CellSizeCm - HalfWidth, Y * CellSizeCm - HalfHeight, Z);
}

void ACWTerrainField::BuildSection(int32 ChunkX, int32 ChunkY, bool bWithCollision)
{
	if (!bGridReady || !TerrainMesh)
	{
		return;
	}

	const int32 StartX = ChunkX * ChunkCells;
	const int32 StartY = ChunkY * ChunkCells;
	const int32 EndX = FMath::Min(StartX + ChunkCells, GridCellsX);
	const int32 EndY = FMath::Min(StartY + ChunkCells, GridCellsY);

	if (StartX >= EndX || StartY >= EndY)
	{
		return;
	}

	const int32 VertsX = EndX - StartX + 1;
	const int32 VertsY = EndY - StartY + 1;
	const int32 VertexCount = VertsX * VertsY;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	Vertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);
	Triangles.Reserve((VertsX - 1) * (VertsY - 1) * 6);

	auto LocalIndex = [VertsX, StartX, StartY](int32 X, int32 Y)
	{
		return (Y - StartY) * VertsX + (X - StartX);
	};

	for (int32 Y = StartY; Y <= EndY; ++Y)
	{
		for (int32 X = StartX; X <= EndX; ++X)
		{
			Vertices.Add(VertexLocalPosition(X, Y));

			const float HeightLeft = Heights[VertexIndex(FMath::Max(0, X - 1), Y)];
			const float HeightRight = Heights[VertexIndex(FMath::Min(GridCellsX, X + 1), Y)];
			const float HeightDown = Heights[VertexIndex(X, FMath::Max(0, Y - 1))];
			const float HeightUp = Heights[VertexIndex(X, FMath::Min(GridCellsY, Y + 1))];

			const FVector Normal(
				-(HeightRight - HeightLeft) / (2.f * CellSizeCm),
				-(HeightUp - HeightDown) / (2.f * CellSizeCm),
				1.f);
			Normals.Add(Normal.GetSafeNormal());

			UVs.Add(FVector2D(
				static_cast<float>(X) / static_cast<float>(GridCellsX),
				static_cast<float>(Y) / static_cast<float>(GridCellsY)));
		}
	}

	for (int32 Y = StartY; Y < EndY; ++Y)
	{
		for (int32 X = StartX; X < EndX; ++X)
		{
			const int32 A = LocalIndex(X, Y);
			const int32 B = LocalIndex(X, Y + 1);
			const int32 C = LocalIndex(X + 1, Y);
			const int32 D = LocalIndex(X + 1, Y + 1);

			Triangles.Add(A);
			Triangles.Add(C);
			Triangles.Add(B);

			Triangles.Add(C);
			Triangles.Add(D);
			Triangles.Add(B);
		}
	}

	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	TerrainMesh->CreateMeshSection(
		ChunkIndex(ChunkX, ChunkY),
		Vertices,
		Triangles,
		Normals,
		UVs,
		Colors,
		Tangents,
		bWithCollision && bGenerateCollision);
}

void ACWTerrainField::RebuildAll(bool bWithCollision)
{
	if (!bGridReady)
	{
		InitializeGrid();
	}

	for (int32 CY = 0; CY < NumChunksY; ++CY)
	{
		for (int32 CX = 0; CX < NumChunksX; ++CX)
		{
			BuildSection(CX, CY, bWithCollision);
		}
	}

	DirtyChunks.Reset();
}

void ACWTerrainField::MarkChunkDirty(int32 ChunkX, int32 ChunkY)
{
	if (ChunkX >= 0 && ChunkX < NumChunksX && ChunkY >= 0 && ChunkY < NumChunksY)
	{
		DirtyChunks.Add(ChunkIndex(ChunkX, ChunkY));
	}
}

void ACWTerrainField::MarkAreaDirty(const FVector& WorldCenter, float RadiusCm)
{
	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return;
	}

	const float RadiusCells = FMath::Max(1.f, RadiusCm / CellSizeCm);
	const int32 MinChunkX = FMath::FloorToInt((GridX - RadiusCells) / ChunkCells);
	const int32 MaxChunkX = FMath::FloorToInt((GridX + RadiusCells) / ChunkCells);
	const int32 MinChunkY = FMath::FloorToInt((GridY - RadiusCells) / ChunkCells);
	const int32 MaxChunkY = FMath::FloorToInt((GridY + RadiusCells) / ChunkCells);

	for (int32 CY = MinChunkY; CY <= MaxChunkY; ++CY)
	{
		for (int32 CX = MinChunkX; CX <= MaxChunkX; ++CX)
		{
			MarkChunkDirty(CX, CY);
		}
	}
}

void ACWTerrainField::ProcessDirtyChunks()
{
	if (DirtyChunks.Num() == 0)
	{
		return;
	}

	TArray<int32> Pending = DirtyChunks.Array();
	int32 Rebuilt = 0;

	for (const int32 SectionIndex : Pending)
	{
		if (Rebuilt >= MaxChunkRebuildsPerTick)
		{
			break;
		}

		const int32 ChunkX = SectionIndex % NumChunksX;
		const int32 ChunkY = SectionIndex / NumChunksX;
		BuildSection(ChunkX, ChunkY, true);
		DirtyChunks.Remove(SectionIndex);
		++Rebuilt;
	}

	CollisionRebuildsLastTick += Rebuilt;
}

int32 ACWTerrainField::FlushDirtyChunks(int32 MaxChunks)
{
	int32 Rebuilt = 0;

	while (DirtyChunks.Num() > 0 && (MaxChunks <= 0 || Rebuilt < MaxChunks))
	{
		const int32 SectionIndex = *DirtyChunks.CreateConstIterator();
		const int32 ChunkX = SectionIndex % NumChunksX;
		const int32 ChunkY = SectionIndex / NumChunksX;
		BuildSection(ChunkX, ChunkY, true);
		DirtyChunks.Remove(SectionIndex);
		++Rebuilt;
	}

	CollisionRebuildsLastTick += Rebuilt;
	return Rebuilt;
}

void ACWTerrainField::UpdateMaterialColor()
{
	if (TerrainMaterial)
	{
		TerrainMaterial->SetVectorParameterValue(
			TEXT("Color"), CWSoil::GetSoilProperties(DefaultSoilType).Color);
	}
}

bool ACWTerrainField::WorldToGrid(const FVector& WorldLocation, float& OutGridX, float& OutGridY) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float HalfWidth = GridCellsX * CellSizeCm * 0.5f;
	const float HalfHeight = GridCellsY * CellSizeCm * 0.5f;

	OutGridX = (Local.X + HalfWidth) / CellSizeCm;
	OutGridY = (Local.Y + HalfHeight) / CellSizeCm;

	return OutGridX >= 0.f && OutGridX <= GridCellsX
		&& OutGridY >= 0.f && OutGridY <= GridCellsY;
}

bool ACWTerrainField::ContainsLocation(const FVector& WorldLocation) const
{
	float GridX = 0.f;
	float GridY = 0.f;
	return WorldToGrid(WorldLocation, GridX, GridY);
}

float ACWTerrainField::GetHeightAtLocation(const FVector& WorldLocation) const
{
	if (!bGridReady)
	{
		return GetActorLocation().Z + ReferenceHeightCm;
	}

	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldLocation, GridX, GridY))
	{
		return GetActorLocation().Z + ReferenceHeightCm;
	}

	const int32 X0 = FMath::Clamp(FMath::FloorToInt(GridX), 0, GridCellsX);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(GridY), 0, GridCellsY);
	const int32 X1 = FMath::Min(X0 + 1, GridCellsX);
	const int32 Y1 = FMath::Min(Y0 + 1, GridCellsY);
	const float TX = GridX - X0;
	const float TY = GridY - Y0;

	const float H00 = Heights[VertexIndex(X0, Y0)];
	const float H10 = Heights[VertexIndex(X1, Y0)];
	const float H01 = Heights[VertexIndex(X0, Y1)];
	const float H11 = Heights[VertexIndex(X1, Y1)];

	const float H0 = FMath::Lerp(H00, H10, TX);
	const float H1 = FMath::Lerp(H01, H11, TX);
	const float LocalHeight = FMath::Lerp(H0, H1, TY);

	const FVector Local(
		GridX * CellSizeCm - GridCellsX * CellSizeCm * 0.5f,
		GridY * CellSizeCm - GridCellsY * CellSizeCm * 0.5f,
		LocalHeight);
	return GetActorTransform().TransformPosition(Local).Z;
}

FVector ACWTerrainField::GetCellCenter(int32 CellX, int32 CellY) const
{
	const int32 X = ClampCellX(CellX);
	const int32 Y = ClampCellY(CellY);

	const float LocalX = (X + 0.5f) * CellSizeCm - GridCellsX * CellSizeCm * 0.5f;
	const float LocalY = (Y + 0.5f) * CellSizeCm - GridCellsY * CellSizeCm * 0.5f;

	const float H = (Heights[VertexIndex(X, Y)]
		+ Heights[VertexIndex(X + 1, Y)]
		+ Heights[VertexIndex(X, Y + 1)]
		+ Heights[VertexIndex(X + 1, Y + 1)]) * 0.25f;

	return GetActorTransform().TransformPosition(FVector(LocalX, LocalY, H));
}

float ACWTerrainField::Falloff(float NormalizedDistance) const
{
	if (NormalizedDistance >= 1.f)
	{
		return 0.f;
	}

	return 0.5f * (1.f + FMath::Cos(PI * FMath::Clamp(NormalizedDistance, 0.f, 1.f)));
}

float ACWTerrainField::ComputeCellVolumeM3(int32 CellX, int32 CellY) const
{
	const int32 X = ClampCellX(CellX);
	const int32 Y = ClampCellY(CellY);

	const float AverageHeight = (Heights[VertexIndex(X, Y)]
		+ Heights[VertexIndex(X + 1, Y)]
		+ Heights[VertexIndex(X, Y + 1)]
		+ Heights[VertexIndex(X + 1, Y + 1)]) * 0.25f;

	const float AreaM2 = (CellSizeCm * CellSizeCm) / 10000.f;
	return (AverageHeight - ReferenceHeightCm) / 100.f * AreaM2;
}

float ACWTerrainField::ComputeAreaVolumeM3(const FVector& WorldCenter, float RadiusCm) const
{
	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return 0.f;
	}

	const float RadiusCells = FMath::Max(0.5f, RadiusCm / CellSizeCm);
	const int32 MinX = FMath::FloorToInt(GridX - RadiusCells);
	const int32 MaxX = FMath::CeilToInt(GridX + RadiusCells);
	const int32 MinY = FMath::FloorToInt(GridY - RadiusCells);
	const int32 MaxY = FMath::CeilToInt(GridY + RadiusCells);

	float Volume = 0.f;
	for (int32 Y = FMath::Max(0, MinY); Y <= FMath::Min(GridCellsY - 1, MaxY); ++Y)
	{
		for (int32 X = FMath::Max(0, MinX); X <= FMath::Min(GridCellsX - 1, MaxX); ++X)
		{
			const float DX = (X + 0.5f) - GridX;
			const float DY = (Y + 0.5f) - GridY;
			if (DX * DX + DY * DY <= RadiusCells * RadiusCells)
			{
				Volume += ComputeCellVolumeM3(X, Y);
			}
		}
	}

	return Volume;
}

float ACWTerrainField::GetVolumeM3InArea(const FVector& WorldCenter, float RadiusCm) const
{
	return ComputeAreaVolumeM3(WorldCenter, RadiusCm);
}

float ACWTerrainField::GetMassKgInArea(const FVector& WorldCenter, float RadiusCm) const
{
	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return 0.f;
	}

	const float RadiusCells = FMath::Max(0.5f, RadiusCm / CellSizeCm);
	const int32 MinX = FMath::FloorToInt(GridX - RadiusCells);
	const int32 MaxX = FMath::CeilToInt(GridX + RadiusCells);
	const int32 MinY = FMath::FloorToInt(GridY - RadiusCells);
	const int32 MaxY = FMath::CeilToInt(GridY + RadiusCells);
	const float AreaM2 = (CellSizeCm * CellSizeCm) / 10000.f;

	float MassKg = 0.f;
	for (int32 Y = FMath::Max(0, MinY); Y <= FMath::Min(GridCellsY - 1, MaxY); ++Y)
	{
		for (int32 X = FMath::Max(0, MinX); X <= FMath::Min(GridCellsX - 1, MaxX); ++X)
		{
			const float DX = (X + 0.5f) - GridX;
			const float DY = (Y + 0.5f) - GridY;
			if (DX * DX + DY * DY > RadiusCells * RadiusCells)
			{
				continue;
			}

			const float AverageHeight = (Heights[VertexIndex(X, Y)]
				+ Heights[VertexIndex(X + 1, Y)]
				+ Heights[VertexIndex(X, Y + 1)]
				+ Heights[VertexIndex(X + 1, Y + 1)]) * 0.25f;
			const float HeightM = FMath::Max(0.f, AverageHeight - ReferenceHeightCm) / 100.f;
			const ECWSoilType Material = Materials[Y * GridCellsX + X];
			MassKg += HeightM * AreaM2 * CWSoil::GetDensityKgPerCubicMeter(Material);
		}
	}

	return MassKg;
}

ECWSoilType ACWTerrainField::GetSoilTypeAtLocation(const FVector& WorldLocation) const
{
	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldLocation, GridX, GridY))
	{
		return DefaultSoilType;
	}

	const int32 X = ClampCellX(FMath::FloorToInt(GridX));
	const int32 Y = ClampCellY(FMath::FloorToInt(GridY));
	return Materials[Y * GridCellsX + X];
}

float ACWTerrainField::GetTotalMassKg() const
{
	if (!bGridReady)
	{
		return 0.f;
	}

	const float AreaM2 = (CellSizeCm * CellSizeCm) / 10000.f;
	float MassKg = 0.f;

	for (int32 Y = 0; Y < GridCellsY; ++Y)
	{
		for (int32 X = 0; X < GridCellsX; ++X)
		{
			const float AverageHeight = (Heights[VertexIndex(X, Y)]
				+ Heights[VertexIndex(X + 1, Y)]
				+ Heights[VertexIndex(X, Y + 1)]
				+ Heights[VertexIndex(X + 1, Y + 1)]) * 0.25f;
			const float HeightM = FMath::Max(0.f, AverageHeight - ReferenceHeightCm) / 100.f;
			const ECWSoilType Material = Materials[Y * GridCellsX + X];
			MassKg += HeightM * AreaM2 * CWSoil::GetDensityKgPerCubicMeter(Material);
		}
	}

	return MassKg;
}

float ACWTerrainField::DigAt(const FVector& WorldCenter, float RadiusCm,
	float MaxDepthCm, float RequestedVolumeM3)
{
	if (!bGridReady || RadiusCm <= 0.f || MaxDepthCm <= 0.f || RequestedVolumeM3 <= 0.f)
	{
		return 0.f;
	}

	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return 0.f;
	}

	const float RadiusCells = RadiusCm / CellSizeCm;
	const int32 MinX = FMath::Max(0, FMath::FloorToInt(GridX - RadiusCells));
	const int32 MaxX = FMath::Min(GridCellsX, FMath::CeilToInt(GridX + RadiusCells));
	const int32 MinY = FMath::Max(0, FMath::FloorToInt(GridY - RadiusCells));
	const int32 MaxY = FMath::Min(GridCellsY, FMath::CeilToInt(GridY + RadiusCells));

	auto VertexFalloff = [this, GridX, GridY, RadiusCells](int32 X, int32 Y)
	{
		const float DX = X - GridX;
		const float DY = Y - GridY;
		const float Distance = FMath::Sqrt(DX * DX + DY * DY) / FMath::Max(0.001f, RadiusCells);
		return Falloff(Distance);
	};

	// Volumen geometrico que se retiraria a profundidad maxima. Se recorren todas
	// las celdas que contienen algun vertice afectado (no solo las celdas cuyo
	// centro cae en el radio): las celdas de borde tambien cambian de altura y su
	// volumen debe entrar en el calculo, o cada cucharada retiraria de mas.
	const float AreaM2 = (CellSizeCm * CellSizeCm) / 10000.f;
	float UnitVolumeM3 = 0.f;
	for (int32 Y = FMath::Max(0, MinY); Y < FMath::Min(GridCellsY, MaxY); ++Y)
	{
		for (int32 X = FMath::Max(0, MinX); X < FMath::Min(GridCellsX, MaxX); ++X)
		{
			const float AverageFalloff = (VertexFalloff(X, Y)
				+ VertexFalloff(X + 1, Y)
				+ VertexFalloff(X, Y + 1)
				+ VertexFalloff(X + 1, Y + 1)) * 0.25f;
			UnitVolumeM3 += AverageFalloff * (MaxDepthCm / 100.f) * AreaM2;
		}
	}

	if (UnitVolumeM3 <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float Scale = FMath::Clamp(RequestedVolumeM3 / UnitVolumeM3, 0.f, 1.f);
	if (Scale <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float BeforeVolume = ComputeAreaVolumeM3(WorldCenter, RadiusCm + CellSizeCm);

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float F = VertexFalloff(X, Y);
			if (F <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			float& Height = Heights[VertexIndex(X, Y)];
			Height = FMath::Max(MinHeightCm, Height - MaxDepthCm * F * Scale);
		}
	}

	const float AfterVolume = ComputeAreaVolumeM3(WorldCenter, RadiusCm + CellSizeCm);
	const float RemovedVolume = FMath::Max(0.f, BeforeVolume - AfterVolume);

	TotalVolumeM3 += (AfterVolume - BeforeVolume);
	MarkAreaDirty(WorldCenter, RadiusCm + CellSizeCm);

	if (RemovedVolume > KINDA_SMALL_NUMBER)
	{
		OnTerrainChanged.Broadcast(WorldCenter, -RemovedVolume);
	}

	return RemovedVolume;
}

float ACWTerrainField::DepositAt(const FVector& WorldCenter, float RadiusCm,
	float VolumeM3, ECWSoilType SoilType)
{
	if (!bGridReady || RadiusCm <= 0.f || VolumeM3 <= 0.f)
	{
		return 0.f;
	}

	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return 0.f;
	}

	const float RadiusCells = RadiusCm / CellSizeCm;
	const int32 MinX = FMath::Max(0, FMath::FloorToInt(GridX - RadiusCells));
	const int32 MaxX = FMath::Min(GridCellsX, FMath::CeilToInt(GridX + RadiusCells));
	const int32 MinY = FMath::Max(0, FMath::FloorToInt(GridY - RadiusCells));
	const int32 MaxY = FMath::Min(GridCellsY, FMath::CeilToInt(GridY + RadiusCells));

	auto VertexFalloff = [this, GridX, GridY, RadiusCells](int32 X, int32 Y)
	{
		const float DX = X - GridX;
		const float DY = Y - GridY;
		const float Distance = FMath::Sqrt(DX * DX + DY * DY) / FMath::Max(0.001f, RadiusCells);
		return Falloff(Distance);
	};

	const float AreaM2 = (CellSizeCm * CellSizeCm) / 10000.f;
	float AreaFactorM3PerCm = 0.f;
	for (int32 Y = FMath::Max(0, MinY); Y < FMath::Min(GridCellsY, MaxY); ++Y)
	{
		for (int32 X = FMath::Max(0, MinX); X < FMath::Min(GridCellsX, MaxX); ++X)
		{
			const float AverageFalloff = (VertexFalloff(X, Y)
				+ VertexFalloff(X + 1, Y)
				+ VertexFalloff(X, Y + 1)
				+ VertexFalloff(X + 1, Y + 1)) * 0.25f;
			AreaFactorM3PerCm += AverageFalloff * AreaM2 / 100.f;
		}
	}

	if (AreaFactorM3PerCm <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float RiseCm = VolumeM3 / AreaFactorM3PerCm;
	const float BeforeVolume = ComputeAreaVolumeM3(WorldCenter, RadiusCm + CellSizeCm);

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float F = VertexFalloff(X, Y);
			if (F <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			float& Height = Heights[VertexIndex(X, Y)];
			Height = FMath::Min(MaxHeightCm, Height + RiseCm * F);
		}
	}

	// El material depositado se marca como suelto en las celdas afectadas.
	for (int32 Y = FMath::Max(0, MinY); Y < FMath::Min(GridCellsY, MaxY); ++Y)
	{
		for (int32 X = FMath::Max(0, MinX); X < FMath::Min(GridCellsX, MaxX); ++X)
		{
			const float DX = (X + 0.5f) - GridX;
			const float DY = (Y + 0.5f) - GridY;
			if (DX * DX + DY * DY <= RadiusCells * RadiusCells)
			{
				Materials[Y * GridCellsX + X] = SoilType;
				CellStates[Y * GridCellsX + X] = 1;
			}
		}
	}

	const float AfterVolume = ComputeAreaVolumeM3(WorldCenter, RadiusCm + CellSizeCm);
	const float AddedVolume = FMath::Max(0.f, AfterVolume - BeforeVolume);

	TotalVolumeM3 += (AfterVolume - BeforeVolume);
	MarkAreaDirty(WorldCenter, RadiusCm + CellSizeCm);

	if (AddedVolume > KINDA_SMALL_NUMBER)
	{
		OnTerrainChanged.Broadcast(WorldCenter, AddedVolume);
	}

	return AddedVolume;
}

bool ACWTerrainField::SetSoilTypeInArea(const FVector& WorldCenter, float RadiusCm, ECWSoilType SoilType)
{
	if (!bGridReady || RadiusCm <= 0.f)
	{
		return false;
	}

	float GridX = 0.f;
	float GridY = 0.f;
	if (!WorldToGrid(WorldCenter, GridX, GridY))
	{
		return false;
	}

	const float RadiusCells = RadiusCm / CellSizeCm;
	const int32 MinX = FMath::Max(0, FMath::FloorToInt(GridX - RadiusCells));
	const int32 MaxX = FMath::Min(GridCellsX - 1, FMath::CeilToInt(GridX + RadiusCells));
	const int32 MinY = FMath::Max(0, FMath::FloorToInt(GridY - RadiusCells));
	const int32 MaxY = FMath::Min(GridCellsY - 1, FMath::CeilToInt(GridY + RadiusCells));

	bool bChanged = false;
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float DX = (X + 0.5f) - GridX;
			const float DY = (Y + 0.5f) - GridY;
			if (DX * DX + DY * DY > RadiusCells * RadiusCells)
			{
				continue;
			}

			const int32 Index = Y * GridCellsX + X;
			if (Materials[Index] != SoilType)
			{
				Materials[Index] = SoilType;
				CellStates[Index] = 1;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		MarkAreaDirty(WorldCenter, RadiusCm);
		OnTerrainChanged.Broadcast(WorldCenter, 0.f);
	}

	return bChanged;
}

void ACWTerrainField::CarveInitialPit()
{
	bCarveInitialPit = true;
	ApplyInitialPit();
}

int64 ACWTerrainField::GetApproxMemoryBytes() const
{
	int64 Bytes = Heights.Num() * static_cast<int64>(sizeof(float));
	Bytes += Materials.Num() * static_cast<int64>(sizeof(ECWSoilType));
	Bytes += CellStates.Num();

	const int32 VertsPerChunk = (ChunkCells + 1) * (ChunkCells + 1);
	const int32 TrisPerChunk = ChunkCells * ChunkCells * 2;
	Bytes += static_cast<int64>(NumChunksX * NumChunksY)
		* (VertsPerChunk * (sizeof(FVector) + sizeof(FVector) + sizeof(FVector2D))
			+ TrisPerChunk * sizeof(int32));

	return Bytes;
}

bool ACWTerrainField::SerializeState(FArchive& Ar, FString* OutError)
{
	if (!bGridReady)
	{
		InitializeGrid();
	}

	FCWTerrainStateHeader Header;
	Header.CellsX = GridCellsX;
	Header.CellsY = GridCellsY;
	Header.CellSizeCm = CellSizeCm;
	Header.ReferenceHeightCm = ReferenceHeightCm;
	Header.MinHeightCm = MinHeightCm;
	Header.MaxHeightCm = MaxHeightCm;
	Header.HeightCount = Heights.Num();
	Header.MaterialCount = Materials.Num();

	// Se guardan las alturas como float para que la carga sea exacta: cuantizar a
	// centimetros introducia varios m3 de error de volumen en 32x32 celdas.
	TArray<float> HeightData;
	TArray<uint8> MaterialBytes;
	TArray<uint8> StateBytes;

	if (Ar.IsSaving())
	{
		HeightData = Heights;

		MaterialBytes.SetNum(Materials.Num());
		for (int32 Index = 0; Index < Materials.Num(); ++Index)
		{
			MaterialBytes[Index] = static_cast<uint8>(Materials[Index]);
		}

		StateBytes = CellStates;

		Header.Checksum = ComputeTerrainChecksum(HeightData, MaterialBytes, StateBytes);
	}
	else
	{
		Header.Checksum = 0;
	}

	Ar.Serialize(&Header, sizeof(FCWTerrainStateHeader));

	if (Ar.IsLoading())
	{
		if (Header.Magic != CWTerrainMagic)
		{
			if (OutError) { *OutError = TEXT("magia de archivo de terreno incorrecta"); }
			return false;
		}
		if (Header.Version != CWTerrainVersion)
		{
			if (OutError) { *OutError = TEXT("version de terreno no soportada"); }
			return false;
		}
		if (Header.CellsX != GridCellsX || Header.CellsY != GridCellsY)
		{
			if (OutError) { *OutError = TEXT("dimensiones del terreno no coinciden"); }
			return false;
		}
		if (Header.HeightCount != Heights.Num() || Header.MaterialCount != Materials.Num())
		{
			if (OutError) { *OutError = TEXT("conteo de datos del terreno incorrecto"); }
			return false;
		}

		HeightData.SetNum(Header.HeightCount);
		MaterialBytes.SetNum(Header.MaterialCount);
		StateBytes.SetNum(Header.MaterialCount);
	}

	Ar.Serialize(HeightData.GetData(), HeightData.Num() * sizeof(float));
	Ar.Serialize(MaterialBytes.GetData(), MaterialBytes.Num() * sizeof(uint8));
	Ar.Serialize(StateBytes.GetData(), StateBytes.Num() * sizeof(uint8));

	if (Ar.IsLoading())
	{
		if (Ar.IsError())
		{
			if (OutError) { *OutError = TEXT("archivo de terreno truncado o ilegible"); }
			return false;
		}

		const uint32 ActualChecksum = ComputeTerrainChecksum(HeightData, MaterialBytes, StateBytes);
		if (ActualChecksum != Header.Checksum)
		{
			if (OutError) { *OutError = TEXT("checksum del terreno incorrecto (archivo corrupto)"); }
			return false;
		}

		for (int32 Index = 0; Index < Heights.Num(); ++Index)
		{
			Heights[Index] = HeightData[Index];
		}

		for (int32 Index = 0; Index < Materials.Num(); ++Index)
		{
			Materials[Index] = static_cast<ECWSoilType>(MaterialBytes[Index]);
		}

		CellStates = StateBytes;
		TotalVolumeM3 = 0.f;
		for (int32 Y = 0; Y < GridCellsY; ++Y)
		{
			for (int32 X = 0; X < GridCellsX; ++X)
			{
				TotalVolumeM3 += ComputeCellVolumeM3(X, Y);
			}
		}

		for (int32 CY = 0; CY < NumChunksY; ++CY)
		{
			for (int32 CX = 0; CX < NumChunksX; ++CX)
			{
				MarkChunkDirty(CX, CY);
			}
		}

		FlushDirtyChunks(0);
		OnTerrainChanged.Broadcast(GetActorLocation(), 0.f);
	}

	return true;
}

bool ACWTerrainField::SaveStateToFile(const FString& FilePath, FString& OutError)
{
	if (FilePath.IsEmpty())
	{
		OutError = TEXT("ruta de guardado vacia");
		return false;
	}

	const FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	FBufferArchive Buffer;
	if (!SerializeState(Buffer, &OutError))
	{
		return false;
	}

	const FString TempPath = FilePath + TEXT(".tmp");
	if (!FFileHelper::SaveArrayToFile(Buffer, *TempPath))
	{
		OutError = FString::Printf(TEXT("no se pudo escribir %s"), *TempPath);
		return false;
	}

	TArray<uint8> Verification;
	if (!FFileHelper::LoadFileToArray(Verification, *TempPath)
		|| Verification.Num() != Buffer.Num())
	{
		OutError = TEXT("la verificacion del archivo temporal fallo");
		return false;
	}

	if (!IFileManager::Get().Move(*FilePath, *TempPath, true, true))
	{
		OutError = FString::Printf(TEXT("no se pudo mover %s a %s"), *TempPath, *FilePath);
		return false;
	}

	UE_LOG(LogCWTerrain, Log, TEXT("CWTerrainField: estado guardado en %s (%d bytes)."),
		*FilePath, Buffer.Num());
	return true;
}

bool ACWTerrainField::LoadStateFromFile(const FString& FilePath, FString& OutError)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *FilePath))
	{
		OutError = FString::Printf(TEXT("no se pudo leer %s"), *FilePath);
		return false;
	}

	if (Data.Num() < static_cast<int32>(sizeof(FCWTerrainStateHeader)))
	{
		OutError = TEXT("archivo de terreno truncado");
		return false;
	}

	FMemoryReader Reader(Data, true);
	if (!SerializeState(Reader, &OutError))
	{
		return false;
	}

	UE_LOG(LogCWTerrain, Log, TEXT("CWTerrainField: estado cargado desde %s (%d bytes)."),
		*FilePath, Data.Num());
	return true;
}
