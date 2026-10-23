// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle/SokobanSolver.h"
#include "Puzzle/SokobanGrid.h"
#include "Algo/Reverse.h"

namespace
{
	/** Order-independent key of a position: the hero cell followed by the sorted crate cells. */
	FString MakeKey(const FSokobanLayout& Layout, const FSokobanBoardState& State)
	{
		TArray<int32, TInlineAllocator<16>> CrateCells;
		for (const FIntPoint& Crate : State.Crates)
		{
			CrateCells.Add(Crate.Y * Layout.Size.X + Crate.X);
		}
		CrateCells.Sort();

		// Cells are stored as characters; +1 keeps them clear of the null terminator.
		FString Key;
		Key.Reserve(CrateCells.Num() + 1);
		const FIntPoint& Hero = State.Heroes[0].Cell;
		Key.AppendChar(static_cast<TCHAR>(1 + Hero.Y * Layout.Size.X + Hero.X));
		for (int32 Cell : CrateCells)
		{
			Key.AppendChar(static_cast<TCHAR>(1 + Cell));
		}
		return Key;
	}

	/**
	 * Floor cells a crate can never leave: corners that are not goals.
	 * Crates are only ever pushed, so a crate wedged between two perpendicular blockers is stuck for good.
	 */
	TArray<bool> FindDeadCorners(const FSokobanLayout& Layout)
	{
		TArray<bool> Dead;
		Dead.Init(false, Layout.Size.X * Layout.Size.Y);
		for (int32 Y = 0; Y < Layout.Size.Y; ++Y)
		{
			for (int32 X = 0; X < Layout.Size.X; ++X)
			{
				const FIntPoint Cell(X, Y);
				if (!Layout.IsFloor(Cell) || Layout.IsGoal(Cell))
				{
					continue;
				}

				auto Blocked = [&Layout, &Cell](ESokobanDirection Direction) { return !Layout.IsFloor(Cell + SokobanGrid::Offset(Direction)); };
				const bool bNorthOrSouth = Blocked(ESokobanDirection::North) || Blocked(ESokobanDirection::South);
				const bool bEastOrWest = Blocked(ESokobanDirection::East) || Blocked(ESokobanDirection::West);
				Dead[Y * Layout.Size.X + X] = bNorthOrSouth && bEastOrWest;
			}
		}
		return Dead;
	}
}

FSokobanSolveResult FSokobanSolver::Solve(const FSokobanLayout& Layout, const FSokobanBoardState& Start, int32 PushStrength, int32 MaxStates)
{
	FSokobanSolveResult Result;
	if (Start.Heroes.Num() == 0)
	{
		return Result;
	}
	if (FSokobanRules::IsSolved(Layout, Start))
	{
		Result.bSolved = true;
		return Result;
	}

	const TArray<bool> DeadCorners = FindDeadCorners(Layout);

	// Breadth-first search; each node remembers its parent and the move that reached it.
	TArray<FSokobanBoardState> States;
	TArray<int32> Parents;
	TArray<ESokobanDirection> Moves;
	TSet<FString> Visited;

	States.Add(Start);
	Parents.Add(INDEX_NONE);
	Moves.Add(ESokobanDirection::North);
	Visited.Add(MakeKey(Layout, Start));

	for (int32 Index = 0; Index < States.Num(); ++Index)
	{
		for (ESokobanDirection Direction : SokobanGrid::AllDirections)
		{
			FSokobanBoardState Next = States[Index];
			const FSokobanMoveResult Move = FSokobanRules::TryMove(Layout, Next, 0, Direction, PushStrength);
			if (!Move.DidMove())
			{
				continue;
			}

			const bool bDeadPush = Move.Row.ContainsByPredicate([&Next, &Layout, &DeadCorners](int32 CrateId)
			{
				const FIntPoint& Crate = Next.Crates[CrateId];
				return DeadCorners[Crate.Y * Layout.Size.X + Crate.X];
			});
			if (bDeadPush)
			{
				continue;
			}

			bool bAlreadyVisited = false;
			Visited.Add(MakeKey(Layout, Next), &bAlreadyVisited);
			if (bAlreadyVisited)
			{
				continue;
			}

			const bool bSolved = FSokobanRules::IsSolved(Layout, Next);
			States.Add(MoveTemp(Next));
			Parents.Add(Index);
			Moves.Add(Direction);

			if (bSolved)
			{
				for (int32 Node = States.Num() - 1; Parents[Node] != INDEX_NONE; Node = Parents[Node])
				{
					Result.Moves.Add(Moves[Node]);
				}
				Algo::Reverse(Result.Moves);
				Result.bSolved = true;
				Result.ExploredStates = States.Num();
				return Result;
			}

			if (States.Num() >= MaxStates)
			{
				Result.ExploredStates = States.Num();
				return Result;
			}
		}
	}

	Result.bProvenUnsolvable = true;
	Result.ExploredStates = States.Num();
	return Result;
}
