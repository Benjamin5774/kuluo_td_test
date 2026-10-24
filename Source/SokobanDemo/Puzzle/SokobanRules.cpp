// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle/SokobanRules.h"
#include "Puzzle/SokobanGrid.h"

FSokobanMoveResult FSokobanRules::TryMove(const FSokobanLayout& Layout, FSokobanBoardState& State, int32 HeroIndex, ESokobanDirection Direction, int32 PushStrength)
{
	FSokobanMoveResult Result;
	Result.HeroIndex = HeroIndex;
	Result.Direction = Direction;
	if (!State.Heroes.IsValidIndex(HeroIndex))
	{
		return Result;
	}

	FSokobanHeroState& Hero = State.Heroes[HeroIndex];
	Hero.Facing = Direction;
	Result.From = Hero.Cell;
	Result.To = Hero.Cell;

	// Trapdoors are judged as they stand before the move: one opens only once its plates are already held.
	const FIntPoint Step = SokobanGrid::Offset(Direction);
	const FIntPoint Target = Hero.Cell + Step;
	if (!CanEnter(Layout, State, Target) || State.FindHeroAt(Target) != INDEX_NONE)
	{
		Result.Outcome = ESokobanMoveOutcome::Blocked;
		return Result;
	}

	// Gather the unbroken row of crates in front of the hero.
	FIntPoint Cell = Target;
	for (int32 CrateId = State.FindCrateAt(Cell); CrateId != INDEX_NONE; CrateId = State.FindCrateAt(Cell))
	{
		Result.Row.Add(CrateId);
		Cell += Step;
	}

	if (Result.Row.Num() > 0)
	{
		if (Result.Row.Num() > PushStrength)
		{
			Result.Outcome = ESokobanMoveOutcome::TooHeavy;
			return Result;
		}

		// Cell is now the first cell past the row; the whole row moves only if it is free floor.
		if (!CanEnter(Layout, State, Cell) || State.FindHeroAt(Cell) != INDEX_NONE)
		{
			Result.Outcome = ESokobanMoveOutcome::Blocked;
			return Result;
		}

		for (int32 CrateId : Result.Row)
		{
			State.Crates[CrateId] += Step;
		}
		Result.Outcome = ESokobanMoveOutcome::Pushed;
	}
	else
	{
		Result.Outcome = ESokobanMoveOutcome::Walked;
	}

	Hero.Cell = Target;
	Result.To = Target;
	++State.MoveCount;
	return Result;
}

bool FSokobanRules::IsSolved(const FSokobanLayout& Layout, const FSokobanBoardState& State)
{
	if (Layout.Goals.Num() == 0)
	{
		return false;
	}

	for (const FIntPoint& Goal : Layout.Goals)
	{
		if (State.FindCrateAt(Goal) == INDEX_NONE)
		{
			return false;
		}
	}
	return true;
}

bool FSokobanRules::ArePlatesHeld(const FSokobanLayout& Layout, const FSokobanBoardState& State)
{
	if (Layout.Plates.Num() == 0)
	{
		return false;
	}

	for (const FIntPoint& Plate : Layout.Plates)
	{
		if (State.FindCrateAt(Plate) == INDEX_NONE && State.FindHeroAt(Plate) == INDEX_NONE)
		{
			return false;
		}
	}
	return true;
}

bool FSokobanRules::IsTrapdoorOpen(const FSokobanLayout& Layout, const FSokobanBoardState& State, const FIntPoint& Cell)
{
	return ArePlatesHeld(Layout, State) || State.FindCrateAt(Cell) != INDEX_NONE || State.FindHeroAt(Cell) != INDEX_NONE;
}

bool FSokobanRules::CanEnter(const FSokobanLayout& Layout, const FSokobanBoardState& State, const FIntPoint& Cell)
{
	return Layout.IsFloor(Cell) && (!Layout.IsTrapdoor(Cell) || IsTrapdoorOpen(Layout, State, Cell));
}

int32 FSokobanRules::StarsForSolve(int32 Moves, int32 Par)
{
	if (Par <= 0)
	{
		return MaxStars;
	}
	if (Moves <= MovesForStars(3, Par))
	{
		return 3;
	}
	return Moves <= MovesForStars(2, Par) ? 2 : 1;
}

int32 FSokobanRules::MovesForStars(int32 Stars, int32 Par)
{
	// Two stars allow half as many moves again as par, rounded up.
	return Stars >= MaxStars ? Par : FMath::CeilToInt(Par * 1.5f);
}
