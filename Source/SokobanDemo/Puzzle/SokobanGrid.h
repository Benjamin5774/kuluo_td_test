// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanTypes.h"

/** Small helpers for working with grid cells and directions. */
namespace SokobanGrid
{
	inline constexpr ESokobanDirection AllDirections[] = { ESokobanDirection::North, ESokobanDirection::East, ESokobanDirection::South, ESokobanDirection::West };

	/** Cell offset of a single step towards Direction. */
	inline FIntPoint Offset(ESokobanDirection Direction)
	{
		switch (Direction)
		{
		case ESokobanDirection::North: return FIntPoint(0, -1);
		case ESokobanDirection::East:  return FIntPoint(1, 0);
		case ESokobanDirection::South: return FIntPoint(0, 1);
		default:                       return FIntPoint(-1, 0);
		}
	}

	inline ESokobanDirection Opposite(ESokobanDirection Direction)
	{
		return static_cast<ESokobanDirection>((static_cast<uint8>(Direction) + 2) % 4);
	}

	/** World yaw in degrees of something facing Direction. The board maps north to +X and east to +Y. */
	inline float ToYaw(ESokobanDirection Direction)
	{
		return 90.f * static_cast<uint8>(Direction);
	}

	/** World-space unit vector of Direction, matching ToYaw. */
	inline FVector ToWorld(ESokobanDirection Direction)
	{
		const FIntPoint Step = Offset(Direction);
		return FVector(-Step.Y, Step.X, 0.f);
	}
}
