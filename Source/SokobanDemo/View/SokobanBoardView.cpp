// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanBoardView.h"
#include "SokobanDemo.h"
#include "Puzzle/SokobanGrid.h"
#include "View/SokobanCrate.h"
#include "View/SokobanHero.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Top of the island body (the grass showing in the seams between floor slabs), relative to the floor. */
	constexpr float IslandTop = -4.f;

	/** How far down the island reaches, well under the sea surface. */
	constexpr float IslandBottom = -400.f;

	/** Pieces from the Quaternius Stylized Nature MegaKit (CC0), in /Game/Sokoban/Environment/Meshes. */
	UStaticMesh* FindEnvironmentMesh(const TCHAR* Name)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(*FString::Printf(TEXT("/Game/Sokoban/Environment/Meshes/%s.%s"), Name, Name));
		return Finder.Object;
	}

	FSokobanDressing MakeDressing(const TCHAR* MeshName, float MinScale, float MaxScale, bool bTall = false)
	{
		FSokobanDressing Dressing;
		Dressing.Mesh = FindEnvironmentMesh(MeshName);
		Dressing.MinScale = MinScale;
		Dressing.MaxScale = MaxScale;
		Dressing.bTall = bTall;
		return Dressing;
	}
}

ASokobanBoardView::ASokobanBoardView()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMaterialAsset(TEXT("/Game/Sokoban/Materials/M_SokobanSolid.M_SokobanSolid"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TerrainMaterialAsset(TEXT("/Game/Sokoban/Environment/Materials/MI_Terrain.MI_Terrain"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShelfMaterialAsset(TEXT("/Game/Sokoban/Environment/Materials/MI_TerrainShelf.MI_TerrainShelf"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterialAsset(TEXT("/Game/Sokoban/Environment/Materials/MI_Water.MI_Water"));
	SolidMaterial = SolidMaterialAsset.Object;
	TerrainMaterial = TerrainMaterialAsset.Object;
	ShelfMaterial = ShelfMaterialAsset.Object;
	WaterMaterial = WaterMaterialAsset.Object;
	FloorMesh = FindEnvironmentMesh(TEXT("RockPath_Square_Wide"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	IslandTiles = CreateLayer(TEXT("IslandTiles"), CubeMesh.Object);
	FloorTiles = CreateLayer(TEXT("FloorTiles"), FloorMesh);
	WallTiles = CreateLayer(TEXT("WallTiles"), CubeMesh.Object);
	GoalSockets = CreateLayer(TEXT("GoalSockets"), CylinderMesh.Object);
	GoalMarkers = CreateLayer(TEXT("GoalMarkers"), CylinderMesh.Object);
	ShelfTiles = CreateLayer(TEXT("ShelfTiles"), CubeMesh.Object);

	// A sea reaching past the edges of the high camera's view.
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	Water->SetupAttachment(RootComponent);
	Water->SetStaticMesh(PlaneMesh.Object);
	Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Water->SetCastShadow(false);
	Water->SetMobility(EComponentMobility::Movable);
	Water->SetRelativeScale3D(FVector(600.f, 600.f, 1.f));

	// The preview copies its meshes from the piece classes when it is built.
	PreviewCrates = CreateLayer(TEXT("PreviewCrates"), nullptr);
	PreviewCrates->bIsEditorOnly = true;
	PreviewHero = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewHero"));
	PreviewHero->SetupAttachment(RootComponent);
	PreviewHero->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewHero->bIsEditorOnly = true;

	HeroClass = ASokobanHero::StaticClass();
	CrateClass = ASokobanCrate::StaticClass();

	// Scales are from the meshes as authored: grass about 1.3 m, bushes 2 m, boulders 3 m, trees 7-9 m.
	WallDressing = {
		MakeDressing(TEXT("Grass_Common_Short"), 0.22f, 0.3f, true),
		MakeDressing(TEXT("Grass_Wispy_Short"), 0.26f, 0.34f, true),
		MakeDressing(TEXT("Plant_1"), 0.24f, 0.32f, true),
		MakeDressing(TEXT("Flower_3_Group"), 0.15f, 0.2f, true),
		MakeDressing(TEXT("Flower_4_Group"), 0.13f, 0.17f, true),
		MakeDressing(TEXT("Bush_Common"), 0.22f, 0.28f, true),
		MakeDressing(TEXT("Bush_Common_Flowers"), 0.22f, 0.28f, true),
		MakeDressing(TEXT("Clover_1"), 0.14f, 0.18f),
		MakeDressing(TEXT("Mushroom_Common"), 0.35f, 0.5f),
		MakeDressing(TEXT("Pebble_Round_1"), 0.5f, 0.85f),
		MakeDressing(TEXT("Pebble_Round_2"), 0.5f, 0.85f),
		MakeDressing(TEXT("Pebble_Square_1"), 0.5f, 0.8f),
	};
	RimDressing = {
		MakeDressing(TEXT("Grass_Common_Short"), 0.18f, 0.26f, true),
		MakeDressing(TEXT("Grass_Wispy_Short"), 0.2f, 0.28f, true),
		MakeDressing(TEXT("Flower_3_Group"), 0.12f, 0.16f, true),
		MakeDressing(TEXT("Flower_4_Group"), 0.1f, 0.14f, true),
		MakeDressing(TEXT("Clover_1"), 0.12f, 0.16f),
		MakeDressing(TEXT("Pebble_Round_2"), 0.4f, 0.6f),
	};
	ShoreRocks = {
		MakeDressing(TEXT("Rock_Medium_1"), 0.25f, 0.4f),
		MakeDressing(TEXT("Rock_Medium_2"), 0.25f, 0.4f),
		MakeDressing(TEXT("Rock_Medium_3"), 0.22f, 0.36f),
	};
	IsletRocks = {
		MakeDressing(TEXT("Rock_Medium_1"), 0.55f, 0.9f),
		MakeDressing(TEXT("Rock_Medium_2"), 0.55f, 0.9f),
		MakeDressing(TEXT("Rock_Medium_3"), 0.5f, 0.8f),
	};
	IsletTrees = {
		MakeDressing(TEXT("CommonTree_1"), 0.3f, 0.42f),
		MakeDressing(TEXT("CommonTree_3"), 0.26f, 0.36f),
		MakeDressing(TEXT("Bush_Common_Flowers"), 0.45f, 0.6f),
	};
}

UInstancedStaticMeshComponent* ASokobanBoardView::CreateLayer(FName Name, UStaticMesh* Mesh)
{
	UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Layer->SetupAttachment(RootComponent);
	Layer->SetStaticMesh(Mesh);
	Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Layer->SetMobility(EComponentMobility::Movable);
	return Layer;
}

void ASokobanBoardView::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// A rerun has just destroyed the dressing layers the previous run made.
	DressingLayers.Reset();

	// The board is otherwise built only in play, which leaves the map empty in the editor.
	const UWorld* World = GetWorld();
	if (World && !World->IsGameWorld())
	{
		BuildEditorPreview();
	}
}

void ASokobanBoardView::BeginPlay()
{
	Super::BeginPlay();

	// A board placed in the map still carries its editor preview, and play-in-editor may have copied the
	// preview's construction-script components (dressing layers, the stand-in's hair) too: start clean.
	TInlineComponentArray<UActorComponent*> Components(this);
	for (UActorComponent* Component : Components)
	{
		if (Component->CreationMethod == EComponentCreationMethod::UserConstructionScript)
		{
			Component->DestroyComponent();
		}
	}
	DressingLayers.Reset();
	ClearEditorPreview();
	ColorLayer(GoalSockets, GoalSocketColor, 0.f);
	ColorLayer(GoalMarkers, GoalColor, GoalGlow);
}

void ASokobanBoardView::ColorLayer(UInstancedStaticMeshComponent* Layer, const FLinearColor& Color, float Glow)
{
	if (UMaterialInstanceDynamic* Material = Layer->CreateDynamicMaterialInstance(0, SolidMaterial))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetScalarParameterValue(TEXT("Glow"), Glow);

		UE_LOG(LogSokoban, Verbose, TEXT("%s uses %s, Color = %s"), *Layer->GetName(), *GetNameSafe(Material->Parent), *Material->K2_GetVectorParameterValue(TEXT("Color")).ToString());
	}
}

void ASokobanBoardView::BuildLevel(const FSokobanLevel& Level)
{
	ClearPieces();
	BuildTerrain(Level);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FSokobanHeroState& HeroState : Level.InitialState.Heroes)
	{
		if (ASokobanHero* Hero = GetWorld()->SpawnActor<ASokobanHero>(HeroClass, CellToWorld(HeroState.Cell), FRotator::ZeroRotator, SpawnParams))
		{
			Hero->SetFacing(HeroState.Facing, true);
			Heroes.Add(Hero);
		}
	}
	for (const FIntPoint& CrateCell : Level.InitialState.Crates)
	{
		if (ASokobanCrate* Crate = GetWorld()->SpawnActor<ASokobanCrate>(CrateClass, CellToWorld(CrateCell), FRotator::ZeroRotator, SpawnParams))
		{
			Crates.Add(Crate);
		}
	}
	UpdateCrateHighlights(Level.InitialState, true);
}

void ASokobanBoardView::ClearTerrain()
{
	IslandTiles->ClearInstances();
	FloorTiles->ClearInstances();
	WallTiles->ClearInstances();
	GoalSockets->ClearInstances();
	GoalMarkers->ClearInstances();
	ShelfTiles->ClearInstances();
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Layer : DressingLayers)
	{
		if (IsValid(Layer.Value))
		{
			Layer.Value->ClearInstances();
		}
	}
}

void ASokobanBoardView::BuildTerrain(const FSokobanLevel& Level)
{
	ClearTerrain();

	Layout = Level.Layout;
	Origin = GetActorLocation() + FVector((Layout.Size.Y - 1) * 0.5f * CellSize, -(Layout.Size.X - 1) * 0.5f * CellSize, 0.f);

	IslandTiles->SetMaterial(0, TerrainMaterial);
	WallTiles->SetMaterial(0, TerrainMaterial);
	ShelfTiles->SetMaterial(0, ShelfMaterial);
	FloorTiles->SetStaticMesh(FloorMesh);
	Water->SetMaterial(0, WaterMaterial);
	Water->SetRelativeLocation(FVector(0.f, 0.f, WaterLevel));

	// Basic shapes are 100 units across with a centred pivot; Unit rescales them to the cell size.
	// Island, wall and shelf blocks fill whole cells so neighbours merge into one mass.
	const float Unit = CellSize / 100.f;
	const FVector IslandScale(Unit, Unit, (IslandTop - IslandBottom) / 100.f);
	const FVector WallScale(Unit, Unit, (WallHeight - IslandTop) / 100.f);

	// The slab is authored about two metres across: fit it into a cell, leaving a seam of grass around it.
	const FBox Slab = FloorMesh ? FloorMesh->GetBoundingBox() : FBox(FVector(-50.f), FVector(50.f));
	const FVector SlabScale(0.94f * CellSize / Slab.GetSize().X, 0.94f * CellSize / Slab.GetSize().Y, 0.55f);
	FRandomStream Random(GetTypeHash(Level.Id));
	FRandomStream ShelfRandom(GetTypeHash(Level.Id) + 2);
	const float IslandZ = (IslandTop + IslandBottom) * 0.5f;
	const float RimDepth = RimWidth * CellSize;

	for (int32 Y = -ShelfWidth; Y < Layout.Size.Y + ShelfWidth; ++Y)
	{
		for (int32 X = -ShelfWidth; X < Layout.Size.X + ShelfWidth; ++X)
		{
			const FIntPoint Cell(X, Y);
			const FVector Center = CellToWorld(Cell);
			if (!IsIsland(Cell))
			{
				// Shallows step down ring by ring away from the coast; the outer rings are patchy.
				const int32 Distance = DistanceToIsland(Cell, ShelfWidth);
				if (Distance <= ShelfWidth && (Distance == 1 || ShelfRandom.FRand() < 0.6f))
				{
					const float Top = ShelfLevel - (Distance - 1) * ShelfStep;
					ShelfTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, (Top + IslandBottom) * 0.5f), FVector(Unit, Unit, (Top - IslandBottom) / 100.f)), true);
				}
				continue;
			}

			IslandTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, IslandZ), IslandScale), true);

			// A grassy rim along the coast, so the floor sits inside the island instead of at its very edge.
			for (ESokobanDirection Direction : SokobanGrid::AllDirections)
			{
				if (IsIsland(Cell + SokobanGrid::Offset(Direction)))
				{
					continue;
				}
				const FVector Out = SokobanGrid::ToWorld(Direction);
				const FVector Size = Out.GetAbs() * RimDepth + FVector(Out.Y, Out.X, 0.f).GetAbs() * CellSize;
				IslandTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + Out * (CellSize + RimDepth) * 0.5f + FVector(0.f, 0.f, IslandZ),
					FVector(Size.X / 100.f, Size.Y / 100.f, IslandScale.Z)), true);
			}
			for (int32 DY = -1; DY <= 1; DY += 2)
			{
				for (int32 DX = -1; DX <= 1; DX += 2)
				{
					// Outer corners, where neither side nor the diagonal continues the island.
					if (IsIsland(Cell + FIntPoint(DX, 0)) || IsIsland(Cell + FIntPoint(0, DY)) || IsIsland(Cell + FIntPoint(DX, DY)))
					{
						continue;
					}
					const FVector Diagonal(-DY, DX, 0.f);
					IslandTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + Diagonal * (CellSize + RimDepth) * 0.5f + FVector(0.f, 0.f, IslandZ),
						FVector(RimDepth / 100.f, RimDepth / 100.f, IslandScale.Z)), true);
				}
			}

			if (Layout.GetTile(Cell) == ESokobanTile::Wall)
			{
				WallTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, (WallHeight + IslandTop) * 0.5f), WallScale), true);
				continue;
			}

			// Quarter turns, plus a hair of jitter, so neighbouring slabs don't repeat; the top sits flush with the floor.
			const FRotator SlabRotation(0.f, 90.f * Random.RandRange(0, 3) + Random.FRandRange(-2.f, 2.f), 0.f);
			FloorTiles->AddInstance(FTransform(SlabRotation, Center - FVector(0.f, 0.f, Slab.Max.Z * SlabScale.Z), SlabScale), true);
			if (Layout.IsGoal(Cell))
			{
				// A gold plate set in a dark stone ring.
				GoalSockets->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, 0.5f * Unit), FVector(0.74f * Unit, 0.74f * Unit, 0.03f * Unit)), true);
				GoalMarkers->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, 1.5f * Unit), FVector(0.52f * Unit, 0.52f * Unit, 0.03f * Unit)), true);
			}
		}
	}

	BuildDressing(Level);
}

bool ASokobanBoardView::IsIsland(const FIntPoint& Cell) const
{
	switch (Layout.GetTile(Cell))
	{
	case ESokobanTile::Floor:
		return true;
	case ESokobanTile::Wall:
		for (int32 DY = -1; DY <= 1; ++DY)
		{
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				// Cells outside the layout count as void too.
				if (Layout.GetTile(Cell + FIntPoint(DX, DY)) == ESokobanTile::Void)
				{
					return false;
				}
			}
		}
		return true;
	default:
		return false;
	}
}

int32 ASokobanBoardView::DistanceToIsland(const FIntPoint& Cell, int32 MaxDistance) const
{
	for (int32 Distance = 1; Distance <= MaxDistance; ++Distance)
	{
		for (int32 DY = -Distance; DY <= Distance; ++DY)
		{
			for (int32 DX = -Distance; DX <= Distance; ++DX)
			{
				if (IsIsland(Cell + FIntPoint(DX, DY)))
				{
					return Distance;
				}
			}
		}
	}
	return MaxDistance + 1;
}

void ASokobanBoardView::BuildDressing(const FSokobanLevel& Level)
{
	// Seeded by the level, so a level always looks the same; offset from the slab rotations' stream.
	FRandomStream Random(GetTypeHash(Level.Id) + 1);

	for (int32 Y = 0; Y < Layout.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Layout.Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			if (!IsIsland(Cell))
			{
				continue;
			}
			const FVector Center = CellToWorld(Cell);

			if (Layout.GetTile(Cell) == ESokobanTile::Wall)
			{
				// The camera looks north from the south: something tall here would hide the floor cell north of it.
				const bool bHidesFloor = Layout.IsFloor(Cell + SokobanGrid::Offset(ESokobanDirection::North));
				for (int32 Count = Random.RandRange(WallDressingCount.X, WallDressingCount.Y); Count > 0; --Count)
				{
					if (const FSokobanDressing* Dressing = PickDressing(WallDressing, bHidesFloor, Random))
					{
						const FVector Jitter(Random.FRandRange(-0.32f, 0.32f) * CellSize, Random.FRandRange(-0.32f, 0.32f) * CellSize, WallHeight);
						AddDressing(*Dressing, Center + Jitter, Random);
					}
				}
			}

			for (ESokobanDirection Direction : SokobanGrid::AllDirections)
			{
				if (IsIsland(Cell + SokobanGrid::Offset(Direction)))
				{
					continue;
				}
				const FVector Out = SokobanGrid::ToWorld(Direction);
				const FVector Along(-Out.Y, Out.X, 0.f);

				// Plants along the rim; only low ones south of a cell, between it and the camera.
				if (Random.FRand() < RimDressingChance)
				{
					if (const FSokobanDressing* Dressing = PickDressing(RimDressing, Direction == ESokobanDirection::South, Random))
					{
						const FVector Location = Center + Out * (0.5f + RimWidth * 0.5f) * CellSize + Along * Random.FRandRange(-0.4f, 0.4f) * CellSize;
						AddDressing(*Dressing, Location + FVector(0.f, 0.f, IslandTop), Random);
					}
				}

				// Boulders just off the rim, resting on the first ring of shallows and poking out of the water.
				if (ShoreRocks.Num() > 0 && Random.FRand() < ShoreRockChance)
				{
					const FVector Location = Center + Out * (0.5f + RimWidth + Random.FRandRange(0.15f, 0.35f)) * CellSize
						+ Along * Random.FRandRange(-0.35f, 0.35f) * CellSize + FVector(0.f, 0.f, ShelfLevel + 10.f);
					AddDressing(ShoreRocks[Random.RandHelper(ShoreRocks.Num())], Location, Random);
				}
			}
		}
	}

	// Islets around the board, kept out of the southern sector the camera looks across.
	const FBox Bounds = GetPlayBounds();
	const float Radius = Bounds.GetExtent().Size2D();
	for (int32 Index = 0; Index < IsletCount && IsletRocks.Num() > 0; ++Index)
	{
		const FSokobanDressing& Rock = IsletRocks[Random.RandHelper(IsletRocks.Num())];
		if (!Rock.Mesh)
		{
			continue;
		}
		const float Angle = FMath::DegreesToRadians(-125.f + 250.f * (Index + Random.FRandRange(0.15f, 0.85f)) / IsletCount);
		const float Distance = Radius + Random.FRandRange(150.f, 700.f);
		const FVector Center = Bounds.GetCenter() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;

		// Sink the rock so its top third shows above the water, then stand a tree on it.
		const float RockScale = Random.FRandRange(Rock.MinScale, Rock.MaxScale);
		const float RockHeight = Rock.Mesh->GetBoundingBox().Max.Z * RockScale;
		const FVector RockBase(Center.X, Center.Y, GetActorLocation().Z + WaterLevel - RockHeight * 0.65f);
		DressingLayer(Rock.Mesh)->AddInstance(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), RockBase, FVector(RockScale)), true);
		if (IsletTrees.Num() > 0 && Random.FRand() < 0.8f)
		{
			AddDressing(IsletTrees[Random.RandHelper(IsletTrees.Num())], RockBase + FVector(0.f, 0.f, RockHeight * 0.85f), Random);
		}
	}
}

const FSokobanDressing* ASokobanBoardView::PickDressing(const TArray<FSokobanDressing>& Choices, bool bLowOnly, FRandomStream& Random)
{
	TArray<const FSokobanDressing*, TInlineAllocator<16>> Candidates;
	for (const FSokobanDressing& Dressing : Choices)
	{
		if (Dressing.Mesh && !(bLowOnly && Dressing.bTall))
		{
			Candidates.Add(&Dressing);
		}
	}
	return Candidates.Num() > 0 ? Candidates[Random.RandHelper(Candidates.Num())] : nullptr;
}

void ASokobanBoardView::AddDressing(const FSokobanDressing& Dressing, const FVector& Location, FRandomStream& Random)
{
	if (!Dressing.Mesh)
	{
		return;
	}
	const float Scale = Random.FRandRange(Dressing.MinScale, Dressing.MaxScale);
	DressingLayer(Dressing.Mesh)->AddInstance(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), Location, FVector(Scale)), true);
}

UInstancedStaticMeshComponent* ASokobanBoardView::DressingLayer(UStaticMesh* Mesh)
{
	if (const TObjectPtr<UInstancedStaticMeshComponent>* Found = DressingLayers.Find(Mesh))
	{
		if (IsValid(*Found))
		{
			return *Found;
		}
	}

	// Layers made for the editor preview are construction-script components, so each rerun clears them.
	const UWorld* World = GetWorld();
	const bool bInPlay = World && World->IsGameWorld();
	UInstancedStaticMeshComponent* Layer = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
	Layer->CreationMethod = bInPlay ? EComponentCreationMethod::Instance : EComponentCreationMethod::UserConstructionScript;
	Layer->SetupAttachment(RootComponent);
	Layer->SetStaticMesh(Mesh);
	Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Layer->SetMobility(EComponentMobility::Movable);
	Layer->RegisterComponent();
	if (bInPlay)
	{
		AddInstanceComponent(Layer);
	}
	DressingLayers.Add(Mesh, Layer);
	return Layer;
}

void ASokobanBoardView::BuildEditorPreview()
{
	ClearEditorPreview();
	if (PreviewLevelIndex < 0)
	{
		ClearTerrain();
		return;
	}

	TArray<FSokobanLevel> Levels;
	FString Error;
	if (!SokobanLevelFiles::LoadPackLevels(SokobanLevelFiles::GetLevelsDir() / PreviewPackFile, Levels, Error) || !Levels.IsValidIndex(PreviewLevelIndex))
	{
		UE_LOG(LogSokoban, Warning, TEXT("Board preview: no level %d in %s. %s"), PreviewLevelIndex, *PreviewPackFile, *Error);
		ClearTerrain();
		return;
	}

	const FSokobanLevel& Level = Levels[PreviewLevelIndex];
	BuildTerrain(Level);
	ColorLayer(GoalSockets, GoalSocketColor, 0.f);
	ColorLayer(GoalMarkers, GoalColor, GoalGlow);

	// Stand-ins look like the real pieces: same meshes and offsets, taken from the piece classes' defaults.
	if (const ASokobanCrate* Crate = CrateClass ? CrateClass->GetDefaultObject<ASokobanCrate>() : nullptr)
	{
		const UStaticMeshComponent* CrateMesh = Crate->GetMesh();
		PreviewCrates->SetStaticMesh(CrateMesh->GetStaticMesh());
		Crate->PaintLook(PreviewCrates);
		for (const FIntPoint& Cell : Level.InitialState.Crates)
		{
			PreviewCrates->AddInstance(CrateMesh->GetRelativeTransform() * FTransform(CellToWorld(Cell)), true);
		}
	}

	const ASokobanHero* Hero = HeroClass ? HeroClass->GetDefaultObject<ASokobanHero>() : nullptr;
	if (Hero && Level.InitialState.Heroes.Num() > 0)
	{
		const FSokobanHeroState& Start = Level.InitialState.Heroes[0];
		const USkeletalMeshComponent* Body = Hero->GetMesh();
		PreviewHero->SetSkeletalMeshAsset(Body->GetSkeletalMeshAsset());
		PreviewHero->SetWorldTransform(Body->GetRelativeTransform() * FTransform(FRotator(0.f, SokobanGrid::ToYaw(Start.Facing), 0.f), CellToWorld(Start.Cell)));
		PreviewHero->OverrideAnimationData(Hero->GetActionClip(ESokobanHeroAction::Idle));
#if WITH_EDITOR
		PreviewHero->SetUpdateAnimationInEditor(true);
#endif
		Hero->AddPartsTo(PreviewHero, EComponentCreationMethod::UserConstructionScript);
	}
}

void ASokobanBoardView::ClearEditorPreview()
{
	// Both are editor-only, so cooked builds don't have them.
	if (PreviewCrates)
	{
		PreviewCrates->ClearInstances();
	}
	if (PreviewHero)
	{
		// The stand-in's hair and beard are follower components attached to it.
		TArray<USceneComponent*> Parts;
		PreviewHero->GetChildrenComponents(false, Parts);
		for (USceneComponent* Part : Parts)
		{
			Part->DestroyComponent();
		}
		PreviewHero->SetSkeletalMeshAsset(nullptr);
	}
}

float ASokobanBoardView::PlayMove(const FSokobanMoveResult& Move, const FSokobanBoardState& NewState)
{
	FinishAnimations();

	ASokobanHero* Hero = Heroes.IsValidIndex(Move.HeroIndex) ? Heroes[Move.HeroIndex].Get() : nullptr;
	if (!Hero)
	{
		return 0.f;
	}

	Hero->SetFacing(Move.Direction, false);
	const FVector Forward = SokobanGrid::ToWorld(Move.Direction);

	switch (Move.Outcome)
	{
	case ESokobanMoveOutcome::Walked:
		Hero->MoveTo(CellToWorld(Move.To), WalkDuration);
		Hero->PlayAction(ESokobanHeroAction::Walk);
		return WalkDuration;

	case ESokobanMoveOutcome::Pushed:
		Hero->MoveTo(CellToWorld(Move.To), PushDuration);
		Hero->PlayAction(ESokobanHeroAction::Push);
		for (int32 CrateId : Move.Row)
		{
			if (Crates.IsValidIndex(CrateId))
			{
				Crates[CrateId]->MoveTo(CellToWorld(NewState.Crates[CrateId]), PushDuration);
			}
		}
		UpdateCrateHighlights(NewState, false);
		return PushDuration;

	default:
		// Refused: the hero leans into the obstacle and the stuck row shudders.
		Hero->Bump(Forward, 18.f, BumpDuration);
		for (int32 CrateId : Move.Row)
		{
			if (Crates.IsValidIndex(CrateId))
			{
				Crates[CrateId]->Bump(Forward, 4.f, BumpDuration);
			}
		}
		return BumpDuration;
	}
}

void ASokobanBoardView::SyncToState(const FSokobanBoardState& State, float Duration)
{
	FinishAnimations();
	const bool bInstant = Duration <= 0.f;

	for (int32 Index = 0; Index < Heroes.Num() && Index < State.Heroes.Num(); ++Index)
	{
		ASokobanHero* Hero = Heroes[Index];
		const FVector Target = CellToWorld(State.Heroes[Index].Cell);
		if (bInstant)
		{
			Hero->SnapTo(Target);
		}
		else
		{
			Hero->MoveTo(Target, Duration);
		}
		Hero->SetFacing(State.Heroes[Index].Facing, bInstant);
		Hero->PlayAction(ESokobanHeroAction::Idle);
	}

	for (int32 Index = 0; Index < Crates.Num() && Index < State.Crates.Num(); ++Index)
	{
		const FVector Target = CellToWorld(State.Crates[Index]);
		if (bInstant)
		{
			Crates[Index]->SnapTo(Target);
		}
		else
		{
			Crates[Index]->MoveTo(Target, Duration);
		}
	}
	UpdateCrateHighlights(State, bInstant);
}

void ASokobanBoardView::FinishAnimations()
{
	for (ASokobanHero* Hero : Heroes)
	{
		Hero->FinishMove();
	}
	for (ASokobanCrate* Crate : Crates)
	{
		Crate->FinishMove();
	}
}

void ASokobanBoardView::PlayVictory()
{
	for (ASokobanHero* Hero : Heroes)
	{
		Hero->PlayAction(ESokobanHeroAction::Victory);
		Hero->Bump(FVector::UpVector, 35.f, 0.35f);
	}
}

void ASokobanBoardView::DumpPieces(const FSokobanBoardState& State) const
{
	for (int32 Index = 0; Index < Crates.Num(); ++Index)
	{
		const ASokobanCrate* Crate = Crates[Index];
		const FIntPoint Cell = State.Crates.IsValidIndex(Index) ? State.Crates[Index] : FIntPoint(-1, -1);
		UE_LOG(LogSokoban, Log, TEXT("Crate %d: cell (%d,%d) expected %s actual %s visual %s hidden=%d moving=%d"), Index, Cell.X, Cell.Y,
			*CellToWorld(Cell).ToCompactString(), *Crate->GetActorLocation().ToCompactString(),
			*Crate->GetRootComponent()->GetChildComponent(0)->GetComponentLocation().ToCompactString(), Crate->IsHidden(), Crate->IsMoving());
	}
}

FVector ASokobanBoardView::CellToWorld(const FIntPoint& Cell) const
{
	// North (row - 1) is +X, east (column + 1) is +Y.
	return Origin + FVector(-Cell.Y * CellSize, Cell.X * CellSize, 0.f);
}

FBox ASokobanBoardView::GetPlayBounds() const
{
	const FVector HalfCell(CellSize * 0.5f, CellSize * 0.5f, 0.f);
	const FVector NorthWest = CellToWorld(FIntPoint(0, 0));
	const FVector SouthEast = CellToWorld(FIntPoint(Layout.Size.X - 1, Layout.Size.Y - 1));
	FBox Bounds(ForceInit);
	Bounds += NorthWest + FVector(HalfCell.X, -HalfCell.Y, 0.f);
	Bounds += SouthEast + FVector(-HalfCell.X, HalfCell.Y, CellSize);
	return Bounds;
}

void ASokobanBoardView::ClearPieces()
{
	for (ASokobanHero* Hero : Heroes)
	{
		if (Hero)
		{
			Hero->Destroy();
		}
	}
	for (ASokobanCrate* Crate : Crates)
	{
		if (Crate)
		{
			Crate->Destroy();
		}
	}
	Heroes.Reset();
	Crates.Reset();
}

void ASokobanBoardView::UpdateCrateHighlights(const FSokobanBoardState& State, bool bInstant)
{
	for (int32 Index = 0; Index < Crates.Num() && Index < State.Crates.Num(); ++Index)
	{
		Crates[Index]->SetOnGoal(Layout.IsGoal(State.Crates[Index]), bInstant);
	}
}
