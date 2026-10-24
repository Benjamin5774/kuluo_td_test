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
 * Draws a level from its layout: the castle (courtyard, walls, towers, goal plates), the rock it stands on with
 * the shallows around it, and the plants that grow on them; and animates the pieces. It only mirrors the puzzle
 * state; all rules live in the Puzzle layer.
 *
 * The scenery around the castle (the sea, islets, trees) is not generated here: it is placed by hand in the map
 * (Outliner folder "Scenery") and stays the same for every level. The sea sits 60 cm below the floor.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanBoardView : public AActor
{
	GENERATED_BODY()

public:
	ASokobanBoardView();

	/** Rebuilds terrain, scenery and pieces for a freshly loaded level. */
	void BuildLevel(const FSokobanLevel& Level);

	/** Animates a move the puzzle has just applied. Returns the seconds from now until it lands. */
	float PlayMove(const FSokobanMoveResult& Move, const FSokobanBoardState& NewState);

	/** Glides every piece to State (undo, restart); instant when Duration is 0. */
	void SyncToState(const FSokobanBoardState& State, float Duration);

	/** Completes all running animations at once, so a new input never waits. */
	void FinishAnimations();

	/** Halts hero Index wherever it is mid-step, after its last step was taken back; the next move sets off from there. */
	void StopHero(int32 Index);

	void PlayVictory();

	/** Dev: logs every piece's grid cell, world location and visibility. */
	void DumpPieces(const FSokobanBoardState& State) const;

	FVector CellToWorld(const FIntPoint& Cell) const;

	/** Bounds of the playable surface, used to frame the camera. */
	FBox GetPlayBounds() const;

	/** Bounds the playable surface of a level Size cells across (columns, rows) would have; levels are centred on the board. */
	FBox GetBoundsForSize(const FIntPoint& Size) const;

	/** The cell under a world position on the board (possibly outside the level). */
	FIntPoint WorldToCell(const FVector& World) const;

	/**
	 * Level editor: faint plates over the board's empty cells, so they can be painted on. Takes effect from the
	 * next BuildLevel; turning it off clears them at once.
	 */
	void SetCanvasOverlay(bool bEnabled);

	/** Level editor: highlights Cell (red when bErase), or nothing when unset or outside the level. */
	void SetCanvasCursor(const TOptional<FIntPoint>& Cell, bool bErase);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Layout")
	float CellSize = 100.f;

	/** Level pack (in Content/Levels) the editor viewport previews, so the scenery around the board can be dressed. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Preview")
	FString PreviewPackFile = TEXT("LevelPack.json");

	/** Index of the previewed level in the pack; negative hides the preview. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Preview")
	int32 PreviewLevelIndex = 0;

	/** Name of the previewed level, for reference. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Sokoban|Preview")
	FString PreviewLevelName;

	/** Shows the next level of the pack in the editor viewport. */
	UFUNCTION(CallInEditor, Category = "Sokoban|Preview")
	void PreviewNextLevel();

	/** Shows the previous level of the pack in the editor viewport. */
	UFUNCTION(CallInEditor, Category = "Sokoban|Preview")
	void PreviewPreviousLevel();

	/** Seconds to walk one cell, one heavy step. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float WalkDuration = 0.25f;

	/** Seconds to push the row one cell once it is sliding: slower than walking, against the weight. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float PushDuration = 0.34f;

	/** Extra seconds a push from a standstill strains before the row gives. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float PushStrain = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float BumpDuration = 0.18f;

	/** A move issued within this many seconds of the previous one landing carries straight on from it. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Timing")
	float FollowOnWindow = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Pieces")
	TSubclassOf<ASokobanHero> HeroClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Pieces")
	TSubclassOf<ASokobanCrate> CrateClass;

	/** Courtyard tile laid on every floor cell, scaled to fit the cell with a seam of mortar around it. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UStaticMesh> FloorMesh;

	/** Dark stone showing in the seams between courtyard tiles. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> MortarMaterial;

	/** The rock the castle stands on: rock on the sides, grass on top. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Castle walls, merlons and towers: brick on the sides, stone on top. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	TObjectPtr<UMaterialInterface> CastleWallMaterial;

	/** Conical roof on the corner towers. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	TObjectPtr<UStaticMesh> TowerRoofMesh;

	/** Height of the outer walls behind and beside the courtyard. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	float CastleWallHeight = 150.f;

	/** Height of the outer wall between the courtyard and the camera, kept low so it hides nothing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	float FrontWallHeight = 30.f;

	/** Side of the merlons along the top of the outer walls. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	float MerlonSize = 30.f;

	/** Round towers on the castle's back corners. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	float TowerRadius = 70.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Castle")
	float TowerHeight = 320.f;

	/** Sand-topped rock under the shallows around the island. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> ShelfMaterial;

	/** Brass pressure plate on every goal; its material reads per-instance custom data 0, which is 1 while pressed. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> GoalPlateMaterial;

	/** The stone ring each plate is set in. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> GoalSocketMaterial;

	/** How far a plate sinks under a crate. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float GoalPressDepth = 1.5f;

	/** How far the walls inside the courtyard rise above the floor. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float WallHeight = 50.f;

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

	/** Grass, flowers, bushes and stones on top of the walls inside the courtyard. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> WallDressing;

	/** Ivy hanging from the castle walls into the courtyard. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	TArray<FSokobanDressing> VineDressing;

	/** Chance per courtyard-facing side of a castle wall block that ivy hangs there. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Dressing")
	float VineChance = 0.3f;

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

	/** Level editor overlay: plates over empty cells, the hovered cell, and the hovered cell while erasing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Editor")
	TObjectPtr<UMaterialInterface> CanvasGridMaterial;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Editor")
	TObjectPtr<UMaterialInterface> CanvasCursorMaterial;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Editor")
	TObjectPtr<UMaterialInterface> CanvasEraseMaterial;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UInstancedStaticMeshComponent* CreateLayer(FName Name, UStaticMesh* Mesh);

	/** Lays out island, floor slabs, walls, goals and shallows; pieces are spawned separately. */
	void BuildTerrain(const FSokobanLevel& Level);
	void ClearTerrain();

	/** Whether Cell is part of the island the castle stands on: anything inside the level's outline. */
	bool IsIsland(const FIntPoint& Cell) const;

	/** Walls touching the outside of the level (diagonals included): the castle's curtain wall. */
	bool IsOuterWall(const FIntPoint& Cell) const;

	/** Outer walls between the courtyard and the camera (which looks north from the south), kept low. */
	bool IsFrontWall(const FIntPoint& Cell) const;

	/** Lays the editor's plates over the empty cells, when the overlay is on. */
	void BuildCanvasOverlay();

public:
	/** How high the top of Cell is above the floor: its wall's height, or 0 for anything else. */
	float WallTop(const FIntPoint& Cell) const;

private:

	bool IsVoid(const FIntPoint& Cell) const { return Layout.GetTile(Cell) == ESokobanTile::Void; }

	/** Chebyshev distance in cells from Cell to the island, or MaxDistance + 1 when farther than that. */
	int32 DistanceToIsland(const FIntPoint& Cell, int32 MaxDistance) const;

	/** Scatters wall and rim dressing and shore rocks; the same level is always dressed the same way. */
	void BuildDressing(const FSokobanLevel& Level);
	void AddDressing(const FSokobanDressing& Dressing, const FVector& Location, FRandomStream& Random);

	/** Picks a dressing at random, only from the low ones when bLowOnly. Null when there is none. */
	static const FSokobanDressing* PickDressing(const TArray<FSokobanDressing>& Choices, bool bLowOnly, FRandomStream& Random);

	/** The instanced layer drawing Mesh, made on first use. */
	UInstancedStaticMeshComponent* DressingLayer(UStaticMesh* Mesh);

	/** Draws PreviewLevelIndex with stand-ins for the pieces, which only exist in play. */
	void BuildEditorPreview();
	void ClearEditorPreview();

	/** Moves the preview Step levels along the pack (wrapping) and redraws it. */
	void StepPreview(int32 Step);

	void ClearPieces();

	/** Lights crates resting on goals and presses the plates under them. */
	void UpdateCrateHighlights(const FSokobanBoardState& State, bool bInstant);

	/** Sinks and lights the plates with a crate on them, raises the others. */
	void PressGoalPlates(const FSokobanBoardState& State);

	FTransform GoalPlateTransform(const FIntPoint& Cell, bool bPressed) const;

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

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> MortarTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> Merlons;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> TowerBodies;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> TowerRoofs;

	/** Underwater rock and sand around the island. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> ShelfTiles;

	/** Level editor overlay (see SetCanvasOverlay and SetCanvasCursor). */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Editor")
	TObjectPtr<UInstancedStaticMeshComponent> CanvasCells;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Editor")
	TObjectPtr<UStaticMeshComponent> CanvasCursor;

	bool bCanvasOverlay = false;

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

	/** The goal under each instance of GoalMarkers, in instance order. */
	TArray<FIntPoint> GoalCells;

	/** Direction of the push the hero made last, while its pushes follow one another; unset otherwise. */
	TOptional<ESokobanDirection> PushingDirection;

	/** World position of the centre of cell (0, 0), the north-west corner. */
	FVector Origin = FVector::ZeroVector;
};
