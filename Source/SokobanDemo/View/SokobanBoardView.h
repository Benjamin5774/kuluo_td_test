// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/SokobanRules.h"
#include "SokobanBoardView.generated.h"

class ASokobanCrate;
class ASokobanHero;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** One kind of decoration the board scatters: a mesh and how much to scale it. */
USTRUCT()
struct FSokobanDressing
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	float MinScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	float MaxScale = 1.f;

	/** Tall pieces stay off walls where they would hide the floor behind them from the camera. */
	UPROPERTY(EditAnywhere, Category = "Sokoban")
	bool bTall = false;
};

/**
 * Draws a level as a small rocky island in the sea, dresses it, and animates the pieces.
 * It only mirrors the puzzle state; all rules live in the Puzzle layer.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanBoardView : public AActor
{
	GENERATED_BODY()

public:
	ASokobanBoardView();

	/** Rebuilds terrain, scenery and pieces for a freshly loaded level. */
	void BuildLevel(const FSokobanLevel& Level);

	/** Animates a move the puzzle has just applied. Returns the animation length in seconds. */
	float PlayMove(const FSokobanMoveResult& Move, const FSokobanBoardState& NewState);

	/** Glides every piece to State (undo, restart); instant when Duration is 0. */
	void SyncToState(const FSokobanBoardState& State, float Duration);

	/** Completes all running animations at once, so a new input never waits. */
	void FinishAnimations();

	void PlayVictory();

	/** Dev: logs every piece's grid cell, world location and visibility. */
	void DumpPieces(const FSokobanBoardState& State) const;

	FVector CellToWorld(const FIntPoint& Cell) const;

	/** Bounds of the playable surface, used to frame the camera. */
	FBox GetPlayBounds() const;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Layout")
	float CellSize = 100.f;

	/** Level pack (in Content/Levels) the editor viewport previews, so the scenery around the board can be dressed. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Preview")
	FString PreviewPackFile = TEXT("LevelPack.json");

	/** Index of the previewed level in the pack; negative hides the preview. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Preview")
	int32 PreviewLevelIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float WalkDuration = 0.14f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float PushDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float BumpDuration = 0.16f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Pieces")
	TSubclassOf<ASokobanHero> HeroClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Pieces")
	TSubclassOf<ASokobanCrate> CrateClass;

	/** Stone slab laid on every floor cell, scaled to fit the cell with a seam of grass around it. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UStaticMesh> FloorMesh;

	/** Island body and wall blocks: rock on the sides, grass on top. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Sand-topped rock under the shallows around the island. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> ShelfMaterial;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	/** Material with Color / Roughness / Glow parameters, used by the goal markers. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> SolidMaterial;

	// Colours are linear (as the material sees them), so they look darker here than on screen.
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor GoalColor = FLinearColor(1.f, 0.62f, 0.04f);

	/** Emissive strength of the goal markers. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float GoalGlow = 1.5f;

	/** The stone ring each goal plate is set in. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor GoalSocketColor = FLinearColor(0.05f, 0.045f, 0.04f);

	/** How far the interior wall blocks rise above the floor. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float WallHeight = 50.f;

	/** Sea level relative to the floor (the floor is at the board's height). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float WaterLevel = -60.f;

	/** Top of the shallows next to the island, relative to the floor; each further ring sits ShelfStep lower. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float ShelfLevel = -115.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float ShelfStep = 40.f;

	/** How many cells of shallows ring the island; rings past the first are patchy. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	int32 ShelfWidth = 2;

	/** Width of the grassy rim the island keeps around its coastal cells, in cells. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float RimWidth = 0.3f;

	/** Grass, flowers, bushes and stones on top of the walls. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> WallDressing;

	/** Pieces of WallDressing per wall block. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	FIntPoint WallDressingCount = FIntPoint(2, 3);

	/** Grass and flowers on the grassy rim around the floor. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> RimDressing;

	/** Chance per coastal edge of a floor cell that a piece of RimDressing grows there. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	float RimDressingChance = 0.6f;

	/** Boulders half sunk along the shore. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> ShoreRocks;

	/** Chance per shore edge of a cell that a boulder sits against it. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	float ShoreRockChance = 0.3f;

	/** Small islands around the board: a rock each, most of them with a tree. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> IsletRocks;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> IsletTrees;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	int32 IsletCount = 9;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UInstancedStaticMeshComponent* CreateLayer(FName Name, UStaticMesh* Mesh);
	void ColorLayer(UInstancedStaticMeshComponent* Layer, const FLinearColor& Color, float Glow);

	/** Lays out island, floor slabs, walls, goals and shallows; pieces are spawned separately. */
	void BuildTerrain(const FSokobanLevel& Level);
	void ClearTerrain();

	/**
	 * Whether Cell is part of the island: floor, and walls enclosed by the level. Walls touching the outside
	 * are drawn as open sea, so the coast follows the shape of the level the way water bounds a puzzle.
	 */
	bool IsIsland(const FIntPoint& Cell) const;

	/** Chebyshev distance in cells from Cell to the island, or MaxDistance + 1 when farther than that. */
	int32 DistanceToIsland(const FIntPoint& Cell, int32 MaxDistance) const;

	/** Scatters wall and rim dressing, shore rocks and islets; the same level always gets the same scenery. */
	void BuildDressing(const FSokobanLevel& Level);
	void AddDressing(const FSokobanDressing& Dressing, const FVector& Location, FRandomStream& Random);

	/** Picks a dressing at random, only from the low ones when bLowOnly. Null when there is none. */
	static const FSokobanDressing* PickDressing(const TArray<FSokobanDressing>& Choices, bool bLowOnly, FRandomStream& Random);

	/** The instanced layer drawing Mesh, made on first use. */
	UInstancedStaticMeshComponent* DressingLayer(UStaticMesh* Mesh);

	/** Draws PreviewLevelIndex with stand-ins for the pieces, which only exist in play. */
	void BuildEditorPreview();
	void ClearEditorPreview();

	void ClearPieces();
	void UpdateCrateHighlights(const FSokobanBoardState& State, bool bInstant);

	/** Body of the island under every non-void cell, reaching down into the sea. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> IslandTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> FloorTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> WallTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> GoalMarkers;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> GoalSockets;

	/** Underwater rock and sand around the island. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> ShelfTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UStaticMeshComponent> Water;

	/** Editor-only stand-ins for the pieces in the preview. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Preview")
	TObjectPtr<UInstancedStaticMeshComponent> PreviewCrates;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewHero;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> DressingLayers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASokobanHero>> Heroes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASokobanCrate>> Crates;

	FSokobanLayout Layout;

	/** World position of the centre of cell (0, 0), the north-west corner. */
	FVector Origin = FVector::ZeroVector;
};
