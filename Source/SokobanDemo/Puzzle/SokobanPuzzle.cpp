// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle/SokobanPuzzle.h"

void FSokobanPuzzle::Load(const FSokobanLevel& InLevel)
{
	Level = InLevel;
	State = Level.InitialState;
	History.Reset();
	ActiveHero = 0;
}

bool FSokobanPuzzle::IsSolved() const
{
	return FSokobanRules::IsSolved(Level.Layout, State);
}

FSokobanMoveResult FSokobanPuzzle::Move(ESokobanDirection Direction)
{
	FSokobanBoardState Before = State;
	const FSokobanMoveResult Result = FSokobanRules::TryMove(Level.Layout, State, ActiveHero, Direction, PushStrength);
	if (Result.DidMove())
	{
		History.Add(MoveTemp(Before));
	}
	return Result;
}

bool FSokobanPuzzle::Undo()
{
	if (History.Num() == 0)
	{
		return false;
	}

	State = History.Pop(EAllowShrinking::No);
	return true;
}

bool FSokobanPuzzle::Restart()
{
	if (State == Level.InitialState)
	{
		return false;
	}

	History.Add(State);
	State = Level.InitialState;
	return true;
}
