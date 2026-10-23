// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanRules.h"

struct SOKOBANDEMO_API FSokobanSolveResult
{
	/** Shortest sequence of moves that solves the level, when bSolved. */
	TArray<ESokobanDirection> Moves;

	bool bSolved = false;

	/** Every reachable position was explored without finding a solution. */
	bool bProvenUnsolvable = false;

	int32 ExploredStates = 0;
};

/** Finds shortest solutions for small levels. Used to validate levels and to replay hints. */
struct SOKOBANDEMO_API FSokobanSolver
{
	/** Breadth-first search over hero and crate positions. Gives up (neither solved nor proven unsolvable) after MaxStates. */
	static FSokobanSolveResult Solve(const FSokobanLayout& Layout, const FSokobanBoardState& Start, int32 PushStrength = FSokobanRules::UnlimitedPush, int32 MaxStates = 500000);
};
