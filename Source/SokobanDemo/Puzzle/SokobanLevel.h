// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanTypes.h"
#include "SokobanLevel.generated.h"

USTRUCT(BlueprintType)
struct FSokobanHeroState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	ESokobanDirection Facing = ESokobanDirection::South;

	bool operator==(const FSokobanHeroState& Other) const
	{
		return Cell == Other.Cell && Facing == Other.Facing;
	}
};

/** Everything that changes while a level is played. The undo history keeps one snapshot per move. */
USTRUCT(BlueprintType)
struct FSokobanBoardState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	TArray<FSokobanHeroState> Heroes;

	/** Crate cells, indexed by a stable crate id so views can follow each crate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	TArray<FIntPoint> Crates;

	/** Moves made to reach this state; undo rewinds it and restart resets it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	int32 MoveCount = 0;

	int32 FindCrateAt(const FIntPoint& Cell) const
	{
		return Crates.IndexOfByKey(Cell);
	}

	int32 FindHeroAt(const FIntPoint& Cell) const
	{
		return Heroes.IndexOfByPredicate([&Cell](const FSokobanHeroState& Hero) { return Hero.Cell == Cell; });
	}

	bool operator==(const FSokobanBoardState& Other) const
	{
		return Heroes == Other.Heroes && Crates == Other.Crates && MoveCount == Other.MoveCount;
	}
};

/** The static part of a level: terrain and goals. */
USTRUCT(BlueprintType)
struct FSokobanLayout
{
	GENERATED_BODY()

	/** Columns (X) and rows (Y). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	FIntPoint Size = FIntPoint::ZeroValue;

	/** Row-major terrain, Size.X * Size.Y entries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	TArray<ESokobanTile> Tiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sokoban")
	TArray<FIntPoint> Goals;

	bool IsInside(const FIntPoint& Cell) const
	{
		return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size.X && Cell.Y < Size.Y;
	}

	/** Cells outside the layout count as void. */
	ESokobanTile GetTile(const FIntPoint& Cell) const
	{
		return IsInside(Cell) ? Tiles[Cell.Y * Size.X + Cell.X] : ESokobanTile::Void;
	}

	bool IsFloor(const FIntPoint& Cell) const
	{
		return GetTile(Cell) == ESokobanTile::Floor;
	}

	bool IsGoal(const FIntPoint& Cell) const
	{
		return Goals.Contains(Cell);
	}
};

/** A level as stored on disk (Content/Levels/*.json). */
USTRUCT(BlueprintType)
struct FSokobanLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	int32 Version = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	FString Name;

	/**
	 * Rows in standard Sokoban (XSB) notation:
	 * '#' wall, '@' hero, '+' hero on goal, '$' crate, '*' crate on goal, '.' goal,
	 * '-' or '_' floor, ' ' open space (floor when reachable from a piece or goal, otherwise void).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	TArray<FString> Map;

	/** Moves in the shortest known solution; 0 when unknown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	int32 Par = 0;
};

/** Ordered list of the levels shipped with the game (Content/Levels/LevelPack.json). */
USTRUCT(BlueprintType)
struct FSokobanLevelPackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	FString Name;

	/** Level file names, relative to the pack file. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sokoban")
	TArray<FString> Levels;
};

/** A parsed, playable level. */
struct SOKOBANDEMO_API FSokobanLevel
{
	FString Id;
	FString Name;
	int32 Par = 0;
	FSokobanLayout Layout;
	FSokobanBoardState InitialState;

	/** Builds a playable level. Returns false with a readable reason when the map is malformed. */
	static bool FromData(const FSokobanLevelData& Data, FSokobanLevel& OutLevel, FString& OutError);

	/** Serializes back to XSB rows, e.g. for the level editor. */
	FSokobanLevelData ToData() const;
};

namespace SokobanLevelFiles
{
	/** Directory holding the shipped levels. */
	SOKOBANDEMO_API FString GetLevelsDir();

	SOKOBANDEMO_API bool LoadLevelData(const FString& FilePath, FSokobanLevelData& OutData, FString& OutError);
	SOKOBANDEMO_API bool SaveLevelData(const FString& FilePath, const FSokobanLevelData& Data, FString& OutError);
	SOKOBANDEMO_API bool LoadLevelPack(const FString& FilePath, FSokobanLevelPackData& OutPack, FString& OutError);

	/** Loads and parses every level listed in a pack, in order. Stops at the first broken level. */
	SOKOBANDEMO_API bool LoadPackLevels(const FString& PackFilePath, TArray<FSokobanLevel>& OutLevels, FString& OutError, FString* OutPackName = nullptr);
}
