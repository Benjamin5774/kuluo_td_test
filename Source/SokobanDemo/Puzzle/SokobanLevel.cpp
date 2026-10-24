// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle/SokobanLevel.h"
#include "Puzzle/SokobanGrid.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/**
	 * Marks every cell reachable from Seeds through cells accepted by CanEnter.
	 * Returns a row-major mask the size of the layout.
	 */
	template <typename CanEnterFn>
	TArray<bool> FloodFrom(const FIntPoint& Size, const TArray<FIntPoint>& Seeds, CanEnterFn CanEnter)
	{
		TArray<bool> Reached;
		Reached.Init(false, Size.X * Size.Y);

		TArray<FIntPoint> Stack;
		for (const FIntPoint& Seed : Seeds)
		{
			Reached[Seed.Y * Size.X + Seed.X] = true;
			Stack.Add(Seed);
		}

		while (Stack.Num() > 0)
		{
			const FIntPoint Cell = Stack.Pop(EAllowShrinking::No);
			for (ESokobanDirection Direction : SokobanGrid::AllDirections)
			{
				const FIntPoint Next = Cell + SokobanGrid::Offset(Direction);
				if (Next.X < 0 || Next.Y < 0 || Next.X >= Size.X || Next.Y >= Size.Y)
				{
					continue;
				}

				const int32 NextIndex = Next.Y * Size.X + Next.X;
				if (!Reached[NextIndex] && CanEnter(NextIndex))
				{
					Reached[NextIndex] = true;
					Stack.Add(Next);
				}
			}
		}
		return Reached;
	}
}

bool FSokobanLevel::FromData(const FSokobanLevelData& Data, FSokobanLevel& OutLevel, FString& OutError)
{
	OutLevel = FSokobanLevel();
	OutLevel.Id = Data.Id;
	OutLevel.Name = Data.Name;
	OutLevel.Par = Data.Par;

	int32 Width = 0;
	for (const FString& Row : Data.Map)
	{
		Width = FMath::Max(Width, Row.Len());
	}
	const int32 Height = Data.Map.Num();
	if (Width == 0 || Height == 0)
	{
		OutError = TEXT("The map is empty.");
		return false;
	}

	FSokobanLayout& Layout = OutLevel.Layout;
	FSokobanBoardState& State = OutLevel.InitialState;
	Layout.Size = FIntPoint(Width, Height);
	Layout.Tiles.Init(ESokobanTile::Void, Width * Height);

	auto AddHero = [&State](const FIntPoint& Cell)
	{
		FSokobanHeroState& Hero = State.Heroes.AddDefaulted_GetRef();
		Hero.Cell = Cell;
		Hero.Facing = ESokobanDirection::South;
	};

	// Explicit floor (pieces, goals, '-' and '_') seeds the flood fill that decides which open spaces are floor.
	TArray<FIntPoint> Seeds;
	TArray<bool> bOpen;
	bOpen.Init(false, Width * Height);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const FString& Row = Data.Map[Y];
		for (int32 X = 0; X < Width; ++X)
		{
			const TCHAR Char = X < Row.Len() ? Row[X] : TEXT(' ');
			const FIntPoint Cell(X, Y);
			const int32 Index = Y * Width + X;

			switch (Char)
			{
			case TEXT('#'):
				Layout.Tiles[Index] = ESokobanTile::Wall;
				continue;
			case TEXT(' '):
				bOpen[Index] = true;
				continue;
			case TEXT('-'):
			case TEXT('_'):
				break;
			case TEXT('.'):
				Layout.Goals.Add(Cell);
				break;
			case TEXT('$'):
				State.Crates.Add(Cell);
				break;
			case TEXT('*'):
				State.Crates.Add(Cell);
				Layout.Goals.Add(Cell);
				break;
			case TEXT('@'):
				AddHero(Cell);
				break;
			case TEXT('+'):
				AddHero(Cell);
				Layout.Goals.Add(Cell);
				break;
			default:
				OutError = FString::Printf(TEXT("Unknown character '%c' at row %d, column %d."), Char, Y + 1, X + 1);
				return false;
			}

			bOpen[Index] = true;
			Seeds.Add(Cell);
		}
	}

	// Mechanisms lie over the map and make their cells floor, like any explicit floor.
	if (Data.Mechanisms.Num() > Height)
	{
		OutError = TEXT("There are more rows of mechanisms than rows in the map.");
		return false;
	}
	for (int32 Y = 0; Y < Data.Mechanisms.Num(); ++Y)
	{
		const FString& Row = Data.Mechanisms[Y];
		for (int32 X = 0; X < Row.Len(); ++X)
		{
			const TCHAR Char = Row[X];
			if (Char == TEXT(' ') || Char == TEXT('-'))
			{
				continue;
			}
			if (Char != TEXT('P') && Char != TEXT('T'))
			{
				OutError = FString::Printf(TEXT("Unknown mechanism '%c' at row %d, column %d."), Char, Y + 1, X + 1);
				return false;
			}

			const FIntPoint Cell(X, Y);
			const int32 Index = Y * Width + X;
			if (X >= Width || Layout.Tiles[Index] == ESokobanTile::Wall || Layout.IsGoal(Cell))
			{
				OutError = FString::Printf(TEXT("The mechanism at row %d, column %d must sit on plain floor (not a wall, a goal or outside the map)."), Y + 1, X + 1);
				return false;
			}
			(Char == TEXT('P') ? Layout.Plates : Layout.Trapdoors).Add(Cell);
			bOpen[Index] = true;
			Seeds.Add(Cell);
		}
	}

	const TArray<bool> Reached = FloodFrom(Layout.Size, Seeds, [&bOpen](int32 Index) { return bOpen[Index]; });
	for (int32 Index = 0; Index < Reached.Num(); ++Index)
	{
		if (Reached[Index])
		{
			Layout.Tiles[Index] = ESokobanTile::Floor;
		}
	}

	if (State.Heroes.Num() == 0)
	{
		OutError = TEXT("The map has no hero ('@' or '+').");
		return false;
	}
	if (State.Heroes.Num() > 1)
	{
		OutError = TEXT("Only one hero per level is supported for now.");
		return false;
	}
	if (Layout.Goals.Num() == 0)
	{
		OutError = TEXT("The map has no goal ('.', '*' or '+').");
		return false;
	}
	if (State.Crates.Num() < Layout.Goals.Num())
	{
		OutError = FString::Printf(TEXT("%d goals but only %d crates: the level can never be solved."), Layout.Goals.Num(), State.Crates.Num());
		return false;
	}
	if (Layout.Trapdoors.Num() > 0 && Layout.Plates.Num() == 0)
	{
		OutError = TEXT("The map has trapdoors ('T') but no pressure plate ('P') to open them.");
		return false;
	}
	return true;
}

FSokobanLevelData FSokobanLevel::ToData() const
{
	FSokobanLevelData Data;
	Data.Id = Id;
	Data.Name = Name;
	Data.Par = Par;

	const FIntPoint& Size = Layout.Size;
	auto IndexOf = [&Size](const FIntPoint& Cell) { return Cell.Y * Size.X + Cell.X; };

	// Floor reachable from a piece, goal or mechanism can be written as plain spaces; any other floor needs an explicit '-'.
	TArray<FIntPoint> Seeds = Layout.Goals;
	Seeds.Append(Layout.Plates);
	Seeds.Append(Layout.Trapdoors);
	Seeds.Append(InitialState.Crates);
	for (const FSokobanHeroState& Hero : InitialState.Heroes)
	{
		Seeds.Add(Hero.Cell);
	}
	const TArray<bool> Reached = FloodFrom(Size, Seeds, [this](int32 Index) { return Layout.Tiles[Index] == ESokobanTile::Floor; });

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		FString Row;
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const bool bGoal = Layout.IsGoal(Cell);
			TCHAR Char = TEXT(' ');
			if (InitialState.FindHeroAt(Cell) != INDEX_NONE)
			{
				Char = bGoal ? TEXT('+') : TEXT('@');
			}
			else if (InitialState.FindCrateAt(Cell) != INDEX_NONE)
			{
				Char = bGoal ? TEXT('*') : TEXT('$');
			}
			else if (bGoal)
			{
				Char = TEXT('.');
			}
			else if (Layout.GetTile(Cell) == ESokobanTile::Wall)
			{
				Char = TEXT('#');
			}
			else if (Layout.GetTile(Cell) == ESokobanTile::Floor && !Reached[IndexOf(Cell)])
			{
				Char = TEXT('-');
			}
			Row.AppendChar(Char);
		}
		Data.Map.Add(Row.TrimEnd());
	}

	if (Layout.Plates.Num() > 0 || Layout.Trapdoors.Num() > 0)
	{
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			FString Row;
			for (int32 X = 0; X < Size.X; ++X)
			{
				const FIntPoint Cell(X, Y);
				Row.AppendChar(Layout.IsPlate(Cell) ? TEXT('P') : (Layout.IsTrapdoor(Cell) ? TEXT('T') : TEXT(' ')));
			}
			Data.Mechanisms.Add(Row.TrimEnd());
		}
	}
	return Data;
}

FString SokobanLevelFiles::GetLevelsDir()
{
	return FPaths::ProjectContentDir() / TEXT("Levels");
}

bool SokobanLevelFiles::LoadLevelData(const FString& FilePath, FSokobanLevelData& OutData, FString& OutError)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FilePath))
	{
		OutError = FString::Printf(TEXT("Cannot read %s"), *FilePath);
		return false;
	}

	FText Reason;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &OutData, 0, 0, false, &Reason))
	{
		OutError = FString::Printf(TEXT("%s: %s"), *FPaths::GetCleanFilename(FilePath), *Reason.ToString());
		return false;
	}
	return true;
}

bool SokobanLevelFiles::SaveLevelData(const FString& FilePath, const FSokobanLevelData& Data, FString& OutError)
{
	FString Json;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Data, Json))
	{
		OutError = TEXT("Failed to convert the level to JSON.");
		return false;
	}
	if (!FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Cannot write %s"), *FilePath);
		return false;
	}
	return true;
}

bool SokobanLevelFiles::LoadLevelPack(const FString& FilePath, FSokobanLevelPackData& OutPack, FString& OutError)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FilePath))
	{
		OutError = FString::Printf(TEXT("Cannot read %s"), *FilePath);
		return false;
	}

	FText Reason;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &OutPack, 0, 0, false, &Reason))
	{
		OutError = FString::Printf(TEXT("%s: %s"), *FPaths::GetCleanFilename(FilePath), *Reason.ToString());
		return false;
	}
	return true;
}

bool SokobanLevelFiles::LoadPackLevels(const FString& PackFilePath, TArray<FSokobanLevel>& OutLevels, FString& OutError, FString* OutPackName)
{
	OutLevels.Reset();

	FSokobanLevelPackData Pack;
	if (!LoadLevelPack(PackFilePath, Pack, OutError))
	{
		return false;
	}
	if (OutPackName)
	{
		*OutPackName = Pack.Name;
	}

	const FString PackDir = FPaths::GetPath(PackFilePath);
	for (const FString& FileName : Pack.Levels)
	{
		FSokobanLevelData Data;
		FSokobanLevel Level;
		FString Error;
		if (!LoadLevelData(PackDir / FileName, Data, Error) || !FSokobanLevel::FromData(Data, Level, Error))
		{
			OutError = FString::Printf(TEXT("%s: %s"), *FileName, *Error);
			return false;
		}
		OutLevels.Add(MoveTemp(Level));
	}

	if (OutLevels.Num() == 0)
	{
		OutError = FString::Printf(TEXT("%s lists no levels."), *FPaths::GetCleanFilename(PackFilePath));
		return false;
	}
	return true;
}
