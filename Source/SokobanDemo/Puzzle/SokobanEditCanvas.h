// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanLevel.h"

/** What a level editor brush paints. */
enum class ESokobanBrush : uint8
{
	Floor,
	Wall,
	Crate,
	Goal,
	Hero,
	/** A pressure plate: holding it down opens the trapdoors. */
	Plate,
	Trapdoor,
	/** Takes away the top-most thing in a cell: a piece, then a goal or mechanism, then the floor or wall itself. */
	Erase
};

/** Something that stops a drawing from being played, in the order the editor reports them. */
enum class ESokobanCanvasIssue : uint8
{
	NoHero,
	NoGoal,
	TooFewCrates,
	/** There are trapdoors but no pressure plate that could open them. */
	TrapdoorWithoutPlate,
	/** Some floor touches the void or the edge of the canvas instead of a wall. */
	NotEnclosed
};

/**
 * A level being drawn in the editor, as a grid of XSB characters plus a grid of mechanisms ('P' plate, 'T' trapdoor,
 * ' ' none). Unlike in level files, ' ' is always void and '-' is always floor, so a draft keeps exactly what was
 * drawn even before it is walled in. Pure logic.
 */
class SOKOBANDEMO_API FSokobanEditCanvas
{
public:
	/** Smallest and largest canvas, in columns and rows. */
	static const FIntPoint MinSize;
	static const FIntPoint MaxSize;

	/** A canvas of Size: floor ringed by walls, the usual starting point. */
	static FSokobanEditCanvas MakeWalledRoom(const FIntPoint& Size);

	/** Exactly the rows of a draft, map and mechanisms (unknown characters become void, or no mechanism). */
	static FSokobanEditCanvas FromRows(const TArray<FString>& Rows, const TArray<FString>& MechanismRows = {});

	/** A playable level (saved or shipped), to draw on further. */
	static FSokobanEditCanvas FromLevel(const FSokobanLevel& Level);

	const FIntPoint& GetSize() const { return Size; }
	bool IsInside(const FIntPoint& Cell) const;

	/** The cell's character; ' ' outside the canvas. */
	TCHAR GetCell(const FIntPoint& Cell) const;

	/** The cell's mechanism: 'P', 'T', or ' ' for none (and outside the canvas). */
	TCHAR GetMechanism(const FIntPoint& Cell) const;

	/** Paints one cell. Returns whether anything changed. */
	bool Paint(const FIntPoint& Cell, ESokobanBrush Brush);

	/** Grows or shrinks to NewSize (clamped to MinSize..MaxSize), keeping the top-left corner; new cells are void. */
	bool Resize(const FIntPoint& NewSize);

	/** Rings every floor region with walls wherever it touches the void. Returns whether anything changed. */
	bool WallIn();

	/** The rows exactly as drawn, for drafts. */
	TArray<FString> ToRows() const;

	/** The mechanism rows, for drafts; empty when there are no mechanisms. */
	TArray<FString> ToMechanismRows() const;

	/** The level as drawn, playable or not (with no hero placed, none appears): for previews and test play. */
	FSokobanLevel ToLevel(const FString& Id, const FString& Name, int32 Par) const;

	/** What stops the drawing from being played; empty when it can be. */
	TArray<ESokobanCanvasIssue> FindIssues() const;

	int32 CountGoals() const;
	int32 CountCrates() const;
	bool HasHero() const;

	bool operator==(const FSokobanEditCanvas& Other) const { return Size == Other.Size && Cells == Other.Cells && Mechanisms == Other.Mechanisms; }
	bool operator!=(const FSokobanEditCanvas& Other) const { return !(*this == Other); }

private:
	int32 IndexOf(const FIntPoint& Cell) const { return Cell.Y * Size.X + Cell.X; }

	FIntPoint Size = FIntPoint::ZeroValue;

	/** Row-major, Size.X * Size.Y characters. */
	TArray<TCHAR> Cells;

	/** Row-major like Cells: 'P', 'T' or ' '. */
	TArray<TCHAR> Mechanisms;
};
