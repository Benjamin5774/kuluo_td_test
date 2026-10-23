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

/**
 * Draws a level as a small floating island and animates the pieces.
 * It only mirrors the puzzle state; all rules live in the Puzzle layer.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanBoardView : public AActor
{
	GENERATED_BODY()

public:
	ASokobanBoardView();

	/** Rebuilds terrain and pieces for a freshly loaded level. */
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

	/** Material with Color / Roughness / Glow parameters shared by every board layer. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> SolidMaterial;

	// Colours are linear (as the material sees them), so they look darker here than on screen.
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor FloorColor = FLinearColor(0.42f, 0.3f, 0.16f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor WallColor = FLinearColor(0.06f, 0.08f, 0.12f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor IslandColor = FLinearColor(0.045f, 0.03f, 0.022f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor GoalColor = FLinearColor(1.f, 0.62f, 0.04f);

	/** Emissive strength of the goal markers. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float GoalGlow = 2.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UInstancedStaticMeshComponent* CreateLayer(FName Name, UStaticMesh* Mesh);
	void ColorLayer(UInstancedStaticMeshComponent* Layer, const FLinearColor& Color, float Glow);
	void ColorLayers();

	/** Lays out island, floor, walls and goals; pieces are spawned separately. */
	void BuildTerrain(const FSokobanLevel& Level);
	void ClearTerrain();

	/** Draws PreviewLevelIndex with stand-ins for the pieces, which only exist in play. */
	void BuildEditorPreview();
	void ClearEditorPreview();

	void ClearPieces();
	void UpdateCrateHighlights(const FSokobanBoardState& State, bool bInstant);

	/** Dark body of the island under every solid cell; its top shows through the gaps between floor tiles. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> IslandTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> FloorTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> WallTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UInstancedStaticMeshComponent> GoalMarkers;

	/** Editor-only stand-ins for the pieces in the preview. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Preview")
	TObjectPtr<UInstancedStaticMeshComponent> PreviewCrates;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewHero;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASokobanHero>> Heroes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASokobanCrate>> Crates;

	FSokobanLayout Layout;

	/** World position of the centre of cell (0, 0), the north-west corner. */
	FVector Origin = FVector::ZeroVector;
};
