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

ASokobanBoardView::ASokobanBoardView()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMaterialAsset(TEXT("/Game/Sokoban/Materials/M_SokobanSolid.M_SokobanSolid"));
	SolidMaterial = SolidMaterialAsset.Object;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	IslandTiles = CreateLayer(TEXT("IslandTiles"), CubeMesh.Object);
	FloorTiles = CreateLayer(TEXT("FloorTiles"), CubeMesh.Object);
	WallTiles = CreateLayer(TEXT("WallTiles"), CubeMesh.Object);
	GoalMarkers = CreateLayer(TEXT("GoalMarkers"), CylinderMesh.Object);

	// The preview copies its meshes from the piece classes when it is built.
	PreviewCrates = CreateLayer(TEXT("PreviewCrates"), nullptr);
	PreviewCrates->bIsEditorOnly = true;
	PreviewHero = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewHero"));
	PreviewHero->SetupAttachment(RootComponent);
	PreviewHero->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewHero->bIsEditorOnly = true;

	HeroClass = ASokobanHero::StaticClass();
	CrateClass = ASokobanCrate::StaticClass();
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

	// A board placed in the map still carries its editor preview; play spawns the real pieces.
	ClearEditorPreview();
	ColorLayers();
}

void ASokobanBoardView::ColorLayers()
{
	ColorLayer(IslandTiles, IslandColor, 0.f);
	ColorLayer(FloorTiles, FloorColor, 0.f);
	ColorLayer(WallTiles, WallColor, 0.f);
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
	GoalMarkers->ClearInstances();
}

void ASokobanBoardView::BuildTerrain(const FSokobanLevel& Level)
{
	ClearTerrain();

	Layout = Level.Layout;
	Origin = GetActorLocation() + FVector((Layout.Size.Y - 1) * 0.5f * CellSize, -(Layout.Size.X - 1) * 0.5f * CellSize, 0.f);

	// Basic shapes are 100 units across with a centred pivot; Unit rescales them to the cell size.
	const float Unit = CellSize / 100.f;
	for (int32 Y = 0; Y < Layout.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Layout.Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const ESokobanTile Tile = Layout.GetTile(Cell);
			if (Tile == ESokobanTile::Void)
			{
				continue;
			}

			const FVector Center = CellToWorld(Cell);
			IslandTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, -70.f * Unit), FVector(Unit, Unit, 1.2f * Unit)), true);

			if (Tile == ESokobanTile::Wall)
			{
				// Low walls (70 units from the island top) keep the playing field readable from the high camera.
				WallTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, 25.f * Unit), FVector(0.98f * Unit, 0.98f * Unit, 0.7f * Unit)), true);
				continue;
			}

			FloorTiles->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, -12.f * Unit), FVector(0.94f * Unit, 0.94f * Unit, 0.24f * Unit)), true);
			if (Layout.IsGoal(Cell))
			{
				GoalMarkers->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, Unit), FVector(0.62f * Unit, 0.62f * Unit, 0.03f * Unit)), true);
			}
		}
	}
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
	ColorLayers();

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
