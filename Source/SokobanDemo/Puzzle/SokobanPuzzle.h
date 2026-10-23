// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanRules.h"

/**
 * A level being played: the live board state plus its undo history.
 * Pure logic, shared by the game and by the level editor's play-test.
 */
class SOKOBANDEMO_API FSokobanPuzzle
{
public:
	/** Crates the hero can push at once. */
	int32 PushStrength = FSokobanRules::UnlimitedPush;

	void Load(const FSokobanLevel& InLevel);

	const FSokobanLevel& GetLevel() const { return Level; }
	const FSokobanBoardState& GetState() const { return State; }
	int32 GetActiveHero() const { return ActiveHero; }
	int32 GetUndoDepth() const { return History.Num(); }
	bool IsSolved() const;

	/** Moves the active hero. A move that changes the board can be undone. */
	FSokobanMoveResult Move(ESokobanDirection Direction);

	/** Rewinds one step. Returns false when there is nothing to undo. */
	bool Undo();

	/** Returns to the starting layout; the restart itself can be undone. Returns false when already there. */
	bool Restart();

private:
	FSokobanLevel Level;
	FSokobanBoardState State;
	TArray<FSokobanBoardState> History;
	int32 ActiveHero = 0;
};
