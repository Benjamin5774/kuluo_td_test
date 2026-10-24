// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanLevel.h"

/** The two shelves the level editor keeps levels on. */
enum class ESokobanShelf : uint8
{
	/** Finished levels, proven solvable, with their par: these can be played. */
	Saved,
	/** Work in progress, stored exactly as drawn (see FSokobanEditCanvas). */
	Drafts
};

/** One level on a shelf. */
struct FSokobanShelfEntry
{
	FSokobanLevelData Data;
	FDateTime Modified;
};

/**
 * Files of the levels made in the editor: one JSON file per level, named after its id, in the same format as
 * the shipped levels. They live under Saved/, which a packaged game can write to.
 */
namespace SokobanShelf
{
	/** Saved/Levels/Saved or Saved/Levels/Drafts. */
	SOKOBANDEMO_API FString GetDir(ESokobanShelf Shelf);

	/** Every level in Dir, most recently changed first. Files that can't be read are skipped. */
	SOKOBANDEMO_API TArray<FSokobanShelfEntry> List(const FString& Dir);

	/** Writes Data to Dir as <Id>.json, replacing an older copy. */
	SOKOBANDEMO_API bool Store(const FString& Dir, const FSokobanLevelData& Data, FString& OutError);

	/** Deletes <Id>.json from Dir. Returns whether there was such a file. */
	SOKOBANDEMO_API bool Remove(const FString& Dir, const FString& Id);

	/** A new, unique level id. */
	SOKOBANDEMO_API FString MakeId();
}
