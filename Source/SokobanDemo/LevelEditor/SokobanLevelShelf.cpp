// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelEditor/SokobanLevelShelf.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
	FString FileOf(const FString& Dir, const FString& Id)
	{
		return Dir / (Id + TEXT(".json"));
	}
}

FString SokobanShelf::GetDir(ESokobanShelf Shelf)
{
	return FPaths::ProjectSavedDir() / TEXT("Levels") / (Shelf == ESokobanShelf::Saved ? TEXT("Saved") : TEXT("Drafts"));
}

TArray<FSokobanShelfEntry> SokobanShelf::List(const FString& Dir)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);

	TArray<FSokobanShelfEntry> Entries;
	for (const FString& File : Files)
	{
		const FString Path = Dir / File;
		FSokobanShelfEntry Entry;
		FString Error;
		if (SokobanLevelFiles::LoadLevelData(Path, Entry.Data, Error))
		{
			// The file name is the id, even if the file says otherwise.
			Entry.Data.Id = FPaths::GetBaseFilename(File);
			Entry.Modified = IFileManager::Get().GetTimeStamp(*Path);
			Entries.Add(MoveTemp(Entry));
		}
	}
	Entries.Sort([](const FSokobanShelfEntry& A, const FSokobanShelfEntry& B) { return A.Modified > B.Modified; });
	return Entries;
}

bool SokobanShelf::Store(const FString& Dir, const FSokobanLevelData& Data, FString& OutError)
{
	if (Data.Id.IsEmpty())
	{
		OutError = TEXT("A level needs an id to be stored.");
		return false;
	}
	IFileManager::Get().MakeDirectory(*Dir, true);
	return SokobanLevelFiles::SaveLevelData(FileOf(Dir, Data.Id), Data, OutError);
}

bool SokobanShelf::Remove(const FString& Dir, const FString& Id)
{
	return !Id.IsEmpty() && IFileManager::Get().Delete(*FileOf(Dir, Id), false, true, true);
}

FString SokobanShelf::MakeId()
{
	return TEXT("custom_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12).ToLower();
}
