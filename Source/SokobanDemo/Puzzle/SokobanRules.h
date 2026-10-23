// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanLevel.h"
#include "SokobanRules.generated.h"

/** What a single move did. Views use it to pick and time animations. */
USTRUCT(BlueprintType)
struct FSokobanMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	ESokobanMoveOutcome Outcome = ESokobanMoveOutcome::Blocked;

	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	int32 HeroIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	ESokobanDirection Direction = ESokobanDirection::South;

	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	FIntPoint From = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	FIntPoint To = FIntPoint::ZeroValue;

	/** Crates in the row in front of the hero, nearest first. They moved one cell when Outcome is Pushed. */
	UPROPERTY(BlueprintReadOnly, Category = "Sokoban")
	TArray<int32> Row;

	bool DidMove() const
	{
		return Outcome == ESokobanMoveOutcome::Walked || Outcome == ESokobanMoveOutcome::Pushed;
	}
};

/** The movement rules, independent of any actor or view. */
struct SOKOBANDEMO_API FSokobanRules
{
	/** Push strength meaning "any number of crates": the strongman's signature ability. */
	static constexpr int32 UnlimitedPush = TNumericLimits<int32>::Max();

	/**
	 * Moves hero HeroIndex one step towards Direction, pushing the whole row of crates in front of it.
	 * The facing always turns towards Direction; everything else changes only when the move succeeds.
	 */
	static FSokobanMoveResult TryMove(const FSokobanLayout& Layout, FSokobanBoardState& State, int32 HeroIndex, ESokobanDirection Direction, int32 PushStrength = UnlimitedPush);

	/** True when every goal holds a crate. */
	static bool IsSolved(const FSokobanLayout& Layout, const FSokobanBoardState& State);
};
