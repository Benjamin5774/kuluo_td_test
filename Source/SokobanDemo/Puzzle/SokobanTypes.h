// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SokobanTypes.generated.h"

/** The four grid directions. Rows grow towards the camera (south), columns grow to the right (east). */
UENUM(BlueprintType)
enum class ESokobanDirection : uint8
{
	North,
	East,
	South,
	West
};

/** Static terrain of one grid cell. */
UENUM(BlueprintType)
enum class ESokobanTile : uint8
{
	/** Outside the island: nothing is drawn and nothing may enter. */
	Void,
	Floor,
	Wall
};

/** Result of trying to move a hero one cell. */
UENUM(BlueprintType)
enum class ESokobanMoveOutcome : uint8
{
	/** Stepped onto a free cell. */
	Walked,
	/** Stepped forward, pushing a row of one or more crates. */
	Pushed,
	/** Facing a wall, the void, another hero, or a row of crates that has nowhere to go. Only the facing changes. */
	Blocked,
	/** The row of crates is longer than this hero can push. Only the facing changes. */
	TooHeavy
};
