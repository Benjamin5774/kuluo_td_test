// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Puzzle/SokobanLevel.h"
#include "Puzzle/SokobanPuzzle.h"
#include "Puzzle/SokobanRules.h"
#include "Puzzle/SokobanSolver.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SokobanTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FSokobanLevelData MakeData(std::initializer_list<const TCHAR*> Rows)
	{
		FSokobanLevelData Data;
		for (const TCHAR* Row : Rows)
		{
			Data.Map.Add(Row);
		}
		return Data;
	}

	bool Parse(FAutomationTestBase& Test, std::initializer_list<const TCHAR*> Rows, FSokobanLevel& OutLevel)
	{
		FString Error;
		const bool bParsed = FSokobanLevel::FromData(MakeData(Rows), OutLevel, Error);
		Test.TestTrue(FString::Printf(TEXT("Map parses (%s)"), *Error), bParsed);
		return bParsed;
	}

	FString ParseError(std::initializer_list<const TCHAR*> Rows)
	{
		FSokobanLevel Level;
		FString Error;
		return FSokobanLevel::FromData(MakeData(Rows), Level, Error) ? FString() : Error;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanParseTest, "Sokoban.Level.Parse", SokobanTest::Flags)
bool FSokobanParseTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, {
		TEXT("  ###  "),
		TEXT("###.###"),
		TEXT("#@ $  #"),
		TEXT("#######") }, Level))
	{
		return false;
	}

	const FSokobanLayout& Layout = Level.Layout;
	TestTrue(TEXT("Size is 7x4"), Layout.Size == FIntPoint(7, 4));
	TestTrue(TEXT("Space outside the walls is void"), Layout.GetTile({ 0, 0 }) == ESokobanTile::Void && Layout.GetTile({ 6, 1 }) == ESokobanTile::Wall);
	TestTrue(TEXT("Space inside the walls is floor"), Layout.IsFloor({ 2, 2 }) && Layout.IsFloor({ 5, 2 }));
	TestTrue(TEXT("Goal is floor and registered"), Layout.IsFloor({ 3, 1 }) && Layout.Goals == TArray<FIntPoint>{ FIntPoint(3, 1) });
	TestTrue(TEXT("Hero found"), Level.InitialState.Heroes.Num() == 1 && Level.InitialState.Heroes[0].Cell == FIntPoint(1, 2));
	TestTrue(TEXT("Crate found"), Level.InitialState.Crates == TArray<FIntPoint>{ FIntPoint(3, 2) });
	TestTrue(TEXT("Outside the layout is void"), Layout.GetTile({ -1, 0 }) == ESokobanTile::Void && Layout.GetTile({ 7, 2 }) == ESokobanTile::Void);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanParseErrorTest, "Sokoban.Level.ParseErrors", SokobanTest::Flags)
bool FSokobanParseErrorTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Empty map is rejected"), SokobanTest::ParseError({}).IsEmpty());
	TestFalse(TEXT("Missing hero is rejected"), SokobanTest::ParseError({ TEXT("#$.#") }).IsEmpty());
	TestFalse(TEXT("Two heroes are rejected"), SokobanTest::ParseError({ TEXT("#@@$.#") }).IsEmpty());
	TestFalse(TEXT("Missing goal is rejected"), SokobanTest::ParseError({ TEXT("#@$ #") }).IsEmpty());
	TestFalse(TEXT("Fewer crates than goals is rejected"), SokobanTest::ParseError({ TEXT("#@$..#") }).IsEmpty());
	TestFalse(TEXT("Unknown character is rejected"), SokobanTest::ParseError({ TEXT("#@$.X#") }).IsEmpty());
	TestTrue(TEXT("Extra crates are allowed"), SokobanTest::ParseError({ TEXT("#@$$.#") }).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanRoundTripTest, "Sokoban.Level.RoundTrip", SokobanTest::Flags)
bool FSokobanRoundTripTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, {
		TEXT("  ######"),
		TEXT("###  .#-#"),
		TEXT("#@ $* ###"),
		TEXT("#########") }, Level))
	{
		return false;
	}
	TestTrue(TEXT("Isolated '-' cell is floor"), Level.Layout.IsFloor({ 7, 1 }));

	const FSokobanLevelData Saved = Level.ToData();
	FSokobanLevel Reloaded;
	FString Error;
	TestTrue(TEXT("Saved map parses again"), FSokobanLevel::FromData(Saved, Reloaded, Error));
	TestTrue(TEXT("Layout survives the round trip"), Reloaded.Layout.Size == Level.Layout.Size && Reloaded.Layout.Tiles == Level.Layout.Tiles && Reloaded.Layout.Goals == Level.Layout.Goals);
	TestTrue(TEXT("Pieces survive the round trip"), Reloaded.InitialState == Level.InitialState);
	TestEqual(TEXT("Reachable floor is written as spaces"), Saved.Map[2], FString(TEXT("#@ $* ###")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanWalkTest, "Sokoban.Rules.Walk", SokobanTest::Flags)
bool FSokobanWalkTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, { TEXT("#######"), TEXT("#@  $.#"), TEXT("#######") }, Level))
	{
		return false;
	}

	FSokobanBoardState State = Level.InitialState;
	FSokobanMoveResult Move = FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::East);
	TestTrue(TEXT("Stepping onto floor walks"), Move.Outcome == ESokobanMoveOutcome::Walked && State.Heroes[0].Cell == FIntPoint(2, 1));
	TestEqual(TEXT("A walk counts as a move"), State.MoveCount, 1);

	Move = FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::North);
	TestTrue(TEXT("Walls block"), Move.Outcome == ESokobanMoveOutcome::Blocked && State.Heroes[0].Cell == FIntPoint(2, 1));
	TestTrue(TEXT("A blocked move still turns the hero"), State.Heroes[0].Facing == ESokobanDirection::North);
	TestEqual(TEXT("A blocked move is not counted"), State.MoveCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanPushRowTest, "Sokoban.Rules.PushRow", SokobanTest::Flags)
bool FSokobanPushRowTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, { TEXT("#########"), TEXT("#@$$$  .#"), TEXT("#########") }, Level))
	{
		return false;
	}

	FSokobanBoardState State = Level.InitialState;
	FSokobanMoveResult Move = FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::East);
	TestTrue(TEXT("The strongman pushes the whole row"), Move.Outcome == ESokobanMoveOutcome::Pushed && Move.Row == TArray<int32>{ 0, 1, 2 });
	TestTrue(TEXT("Every crate in the row moved"), State.Crates == TArray<FIntPoint>{ FIntPoint(3, 1), FIntPoint(4, 1), FIntPoint(5, 1) });
	TestTrue(TEXT("The hero follows the row"), State.Heroes[0].Cell == FIntPoint(2, 1));

	FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::East);
	FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::East);
	const FSokobanBoardState BeforeWall = State;
	Move = FSokobanRules::TryMove(Level.Layout, State, 0, ESokobanDirection::East);
	TestTrue(TEXT("A row against a wall does not move"), Move.Outcome == ESokobanMoveOutcome::Blocked && State.Crates == BeforeWall.Crates && State.Heroes[0].Cell == BeforeWall.Heroes[0].Cell);
	TestTrue(TEXT("A blocked push reports the row"), Move.Row.Num() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanPushStrengthTest, "Sokoban.Rules.PushStrength", SokobanTest::Flags)
bool FSokobanPushStrengthTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, { TEXT("#######"), TEXT("#@$$ .#"), TEXT("#######") }, Level))
	{
		return false;
	}

	FSokobanBoardState Weak = Level.InitialState;
	const FSokobanMoveResult WeakMove = FSokobanRules::TryMove(Level.Layout, Weak, 0, ESokobanDirection::East, 1);
	TestTrue(TEXT("A row longer than the push strength is too heavy"), WeakMove.Outcome == ESokobanMoveOutcome::TooHeavy && Weak.Crates == Level.InitialState.Crates);

	FSokobanBoardState Strong = Level.InitialState;
	const FSokobanMoveResult StrongMove = FSokobanRules::TryMove(Level.Layout, Strong, 0, ESokobanDirection::East, 2);
	TestTrue(TEXT("A row within the push strength moves"), StrongMove.Outcome == ESokobanMoveOutcome::Pushed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanUndoRestartTest, "Sokoban.Puzzle.UndoRestart", SokobanTest::Flags)
bool FSokobanUndoRestartTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, { TEXT("#######"), TEXT("#@ $ .#"), TEXT("#######") }, Level))
	{
		return false;
	}

	FSokobanPuzzle Puzzle;
	Puzzle.Load(Level);
	TestFalse(TEXT("Nothing to undo at the start"), Puzzle.Undo());
	TestFalse(TEXT("Restart at the start does nothing"), Puzzle.Restart());

	Puzzle.Move(ESokobanDirection::East);
	const FSokobanBoardState AfterFirstMove = Puzzle.GetState();
	Puzzle.Move(ESokobanDirection::East);
	Puzzle.Move(ESokobanDirection::North);
	TestEqual(TEXT("Blocked moves are not recorded"), Puzzle.GetUndoDepth(), 2);

	TestTrue(TEXT("Undo succeeds"), Puzzle.Undo());
	TestTrue(TEXT("Undo restores the previous position"), Puzzle.GetState() == AfterFirstMove);

	TestTrue(TEXT("Restart succeeds"), Puzzle.Restart());
	TestTrue(TEXT("Restart returns to the start"), Puzzle.GetState() == Level.InitialState);
	TestTrue(TEXT("A restart can be undone"), Puzzle.Undo() && Puzzle.GetState() == AfterFirstMove);

	TestTrue(TEXT("Undo back to the start"), Puzzle.Undo() && Puzzle.GetState() == Level.InitialState);
	TestFalse(TEXT("History is exhausted"), Puzzle.Undo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanSolvedTest, "Sokoban.Rules.Solved", SokobanTest::Flags)
bool FSokobanSolvedTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Level;
	if (!SokobanTest::Parse(*this, { TEXT("######"), TEXT("#@$ .#"), TEXT("######") }, Level))
	{
		return false;
	}

	FSokobanPuzzle Puzzle;
	Puzzle.Load(Level);
	Puzzle.Move(ESokobanDirection::East);
	TestFalse(TEXT("Not solved while a goal is empty"), Puzzle.IsSolved());
	Puzzle.Move(ESokobanDirection::East);
	TestTrue(TEXT("Solved once every goal holds a crate"), Puzzle.IsSolved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanSolverTest, "Sokoban.Solver.Basics", SokobanTest::Flags)
bool FSokobanSolverTest::RunTest(const FString& Parameters)
{
	FSokobanLevel Easy;
	if (SokobanTest::Parse(*this, { TEXT("########"), TEXT("#      #"), TEXT("# @ $ .#"), TEXT("#      #"), TEXT("########") }, Easy))
	{
		const FSokobanSolveResult Result = FSokobanSolver::Solve(Easy.Layout, Easy.InitialState);
		TestTrue(TEXT("Easy level is solved"), Result.bSolved);
		TestEqual(TEXT("Shortest solution length"), Result.Moves.Num(), 3);

		FSokobanPuzzle Puzzle;
		Puzzle.Load(Easy);
		for (ESokobanDirection Move : Result.Moves)
		{
			Puzzle.Move(Move);
		}
		TestTrue(TEXT("Replaying the solution solves the puzzle"), Puzzle.IsSolved());
	}

	FSokobanLevel Stuck;
	if (SokobanTest::Parse(*this, { TEXT("######"), TEXT("#@  .#"), TEXT("#$   #"), TEXT("######") }, Stuck))
	{
		const FSokobanSolveResult Result = FSokobanSolver::Solve(Stuck.Layout, Stuck.InitialState);
		TestTrue(TEXT("A crate stuck in a corner is proven unsolvable"), !Result.bSolved && Result.bProvenUnsolvable);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanBuiltinLevelsTest, "Sokoban.Levels.Builtin", SokobanTest::Flags)
bool FSokobanBuiltinLevelsTest::RunTest(const FString& Parameters)
{
	TArray<FSokobanLevel> Levels;
	FString Error;
	const FString PackFile = SokobanLevelFiles::GetLevelsDir() / TEXT("LevelPack.json");
	if (!SokobanLevelFiles::LoadPackLevels(PackFile, Levels, Error))
	{
		AddError(Error);
		return false;
	}

	for (const FSokobanLevel& Level : Levels)
	{
		const FSokobanSolveResult Result = FSokobanSolver::Solve(Level.Layout, Level.InitialState);
		if (!Result.bSolved)
		{
			AddError(FString::Printf(TEXT("Level %s (%s) is not solvable (%s)."), *Level.Id, *Level.Name, Result.bProvenUnsolvable ? TEXT("proven") : TEXT("search gave up")));
			continue;
		}

		AddInfo(FString::Printf(TEXT("Level %s (%s): shortest solution %d moves, %d states explored."), *Level.Id, *Level.Name, Result.Moves.Num(), Result.ExploredStates));
		if (Level.Par > 0 && Level.Par != Result.Moves.Num())
		{
			AddError(FString::Printf(TEXT("Level %s declares par %d but the shortest solution takes %d moves."), *Level.Id, Level.Par, Result.Moves.Num()));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
