// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle/SokobanEditCanvas.h"
#include "Puzzle/SokobanGrid.h"

const FIntPoint FSokobanEditCanvas::MinSize(3, 3);
const FIntPoint FSokobanEditCanvas::MaxSize(12, 9);

namespace
{
	/** A canvas cell taken apart: what it is made of, whether it is a goal, its mechanism and the piece standing on it. */
	struct FCellParts
	{
		ESokobanTile Base = ESokobanTile::Void;
		bool bGoal = false;
		/** '$' a crate, '@' the hero, 0 nothing. */
		TCHAR Piece = 0;
		/** 'P' a pressure plate, 'B' a barrier, 0 nothing. Only on floor that is no goal. */
		TCHAR Mechanism = 0;
	};

	FCellParts Decode(TCHAR Char, TCHAR Mechanism = TEXT(' '))
	{
		FCellParts Parts;
		switch (Char)
		{
		case TEXT('#'):
			Parts.Base = ESokobanTile::Wall;
			break;
		case TEXT('-'):
			Parts.Base = ESokobanTile::Floor;
			break;
		case TEXT('.'):
			Parts = { ESokobanTile::Floor, true, 0 };
			break;
		case TEXT('$'):
			Parts = { ESokobanTile::Floor, false, TEXT('$') };
			break;
		case TEXT('*'):
			Parts = { ESokobanTile::Floor, true, TEXT('$') };
			break;
		case TEXT('@'):
			Parts = { ESokobanTile::Floor, false, TEXT('@') };
			break;
		case TEXT('+'):
			Parts = { ESokobanTile::Floor, true, TEXT('@') };
			break;
		default:
			break;
		}
		if (Parts.Base == ESokobanTile::Floor && !Parts.bGoal && (Mechanism == TEXT('P') || Mechanism == TEXT('B')))
		{
			Parts.Mechanism = Mechanism;
		}
		return Parts;
	}

	TCHAR Encode(const FCellParts& Parts)
	{
		if (Parts.Base == ESokobanTile::Wall)
		{
			return TEXT('#');
		}
		if (Parts.Base == ESokobanTile::Void)
		{
			return TEXT(' ');
		}
		if (Parts.Piece == TEXT('$'))
		{
			return Parts.bGoal ? TEXT('*') : TEXT('$');
		}
		if (Parts.Piece == TEXT('@'))
		{
			return Parts.bGoal ? TEXT('+') : TEXT('@');
		}
		return Parts.bGoal ? TEXT('.') : TEXT('-');
	}

	TCHAR EncodeMechanism(const FCellParts& Parts)
	{
		return Parts.Base == ESokobanTile::Floor && !Parts.bGoal && Parts.Mechanism != 0 ? Parts.Mechanism : TEXT(' ');
	}

	FIntPoint ClampSize(const FIntPoint& Size)
	{
		return FIntPoint(FMath::Clamp(Size.X, FSokobanEditCanvas::MinSize.X, FSokobanEditCanvas::MaxSize.X),
			FMath::Clamp(Size.Y, FSokobanEditCanvas::MinSize.Y, FSokobanEditCanvas::MaxSize.Y));
	}
}

FSokobanEditCanvas FSokobanEditCanvas::MakeWalledRoom(const FIntPoint& InSize)
{
	FSokobanEditCanvas Canvas;
	Canvas.Size = ClampSize(InSize);
	Canvas.Cells.Init(TEXT('-'), Canvas.Size.X * Canvas.Size.Y);
	Canvas.Mechanisms.Init(TEXT(' '), Canvas.Size.X * Canvas.Size.Y);
	for (int32 Y = 0; Y < Canvas.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Canvas.Size.X; ++X)
		{
			if (X == 0 || Y == 0 || X == Canvas.Size.X - 1 || Y == Canvas.Size.Y - 1)
			{
				Canvas.Cells[Canvas.IndexOf(FIntPoint(X, Y))] = TEXT('#');
			}
		}
	}
	return Canvas;
}

FSokobanEditCanvas FSokobanEditCanvas::FromRows(const TArray<FString>& Rows, const TArray<FString>& MechanismRows)
{
	FSokobanEditCanvas Canvas;
	int32 Width = 0;
	for (const FString& Row : Rows)
	{
		Width = FMath::Max(Width, Row.Len());
	}
	Canvas.Size = FIntPoint(Width, Rows.Num());
	Canvas.Cells.Init(TEXT(' '), Width * Rows.Num());
	Canvas.Mechanisms.Init(TEXT(' '), Width * Rows.Num());
	for (int32 Y = 0; Y < Rows.Num(); ++Y)
	{
		for (int32 X = 0; X < Rows[Y].Len(); ++X)
		{
			const TCHAR Char = Rows[Y][X] == TEXT('_') ? TEXT('-') : Rows[Y][X];
			const TCHAR Mechanism = MechanismRows.IsValidIndex(Y) && X < MechanismRows[Y].Len() ? MechanismRows[Y][X] : TEXT(' ');
			// Round-tripping through Decode turns anything unknown into void, or into no mechanism.
			const FCellParts Parts = Decode(Char, Mechanism);
			Canvas.Cells[Canvas.IndexOf(FIntPoint(X, Y))] = Encode(Parts);
			Canvas.Mechanisms[Canvas.IndexOf(FIntPoint(X, Y))] = EncodeMechanism(Parts);
		}
	}
	return Canvas;
}

FSokobanEditCanvas FSokobanEditCanvas::FromLevel(const FSokobanLevel& Level)
{
	FSokobanEditCanvas Canvas;
	Canvas.Size = Level.Layout.Size;
	Canvas.Cells.Init(TEXT(' '), Canvas.Size.X * Canvas.Size.Y);
	Canvas.Mechanisms.Init(TEXT(' '), Canvas.Size.X * Canvas.Size.Y);
	for (int32 Y = 0; Y < Canvas.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Canvas.Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			FCellParts Parts;
			Parts.Base = Level.Layout.GetTile(Cell);
			if (Parts.Base == ESokobanTile::Floor)
			{
				Parts.bGoal = Level.Layout.IsGoal(Cell);
				Parts.Mechanism = Level.Layout.IsPlate(Cell) ? TEXT('P') : (Level.Layout.IsBarrier(Cell) ? TEXT('B') : 0);
				if (Level.InitialState.FindHeroAt(Cell) != INDEX_NONE)
				{
					Parts.Piece = TEXT('@');
				}
				else if (Level.InitialState.FindCrateAt(Cell) != INDEX_NONE)
				{
					Parts.Piece = TEXT('$');
				}
			}
			Canvas.Cells[Canvas.IndexOf(Cell)] = Encode(Parts);
			Canvas.Mechanisms[Canvas.IndexOf(Cell)] = EncodeMechanism(Parts);
		}
	}
	return Canvas;
}

bool FSokobanEditCanvas::IsInside(const FIntPoint& Cell) const
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size.X && Cell.Y < Size.Y;
}

bool FSokobanEditCanvas::IsEdge(const FIntPoint& Cell) const
{
	return IsInside(Cell) && (Cell.X == 0 || Cell.Y == 0 || Cell.X == Size.X - 1 || Cell.Y == Size.Y - 1);
}

bool FSokobanEditCanvas::CanPaint(const FIntPoint& Cell, ESokobanBrush Brush) const
{
	return IsInside(Cell) && (!IsEdge(Cell) || Brush == ESokobanBrush::Wall || Brush == ESokobanBrush::Erase);
}

TCHAR FSokobanEditCanvas::GetCell(const FIntPoint& Cell) const
{
	return IsInside(Cell) ? Cells[IndexOf(Cell)] : TEXT(' ');
}

TCHAR FSokobanEditCanvas::GetMechanism(const FIntPoint& Cell) const
{
	return IsInside(Cell) ? Mechanisms[IndexOf(Cell)] : TEXT(' ');
}

bool FSokobanEditCanvas::Paint(const FIntPoint& Cell, ESokobanBrush Brush)
{
	if (!CanPaint(Cell, Brush))
	{
		return false;
	}

	const TArray<TCHAR> Before = Cells;
	const TArray<TCHAR> MechanismsBefore = Mechanisms;
	if (Brush == ESokobanBrush::Hero)
	{
		// There is only one hero: placing it somewhere takes it away from wherever it stood.
		for (TCHAR& Other : Cells)
		{
			Other = Other == TEXT('@') ? TEXT('-') : (Other == TEXT('+') ? TEXT('.') : Other);
		}
	}

	const int32 Index = IndexOf(Cell);
	FCellParts Parts = Decode(Cells[Index], Mechanisms[Index]);
	switch (Brush)
	{
	case ESokobanBrush::Floor:
		Parts.Base = ESokobanTile::Floor;
		Parts.bGoal = false;
		Parts.Mechanism = 0;
		break;
	case ESokobanBrush::Wall:
		Parts = FCellParts();
		Parts.Base = ESokobanTile::Wall;
		break;
	case ESokobanBrush::Goal:
		// A goal, a plate and a barrier each take the whole floor of a cell.
		Parts.Base = ESokobanTile::Floor;
		Parts.bGoal = true;
		Parts.Mechanism = 0;
		break;
	case ESokobanBrush::Crate:
		Parts.Base = ESokobanTile::Floor;
		Parts.Piece = TEXT('$');
		break;
	case ESokobanBrush::Hero:
		Parts.Base = ESokobanTile::Floor;
		Parts.Piece = TEXT('@');
		break;
	case ESokobanBrush::Plate:
	case ESokobanBrush::Barrier:
		Parts.Base = ESokobanTile::Floor;
		Parts.bGoal = false;
		Parts.Mechanism = Brush == ESokobanBrush::Plate ? TEXT('P') : TEXT('B');
		break;
	case ESokobanBrush::Erase:
		if (Parts.Piece != 0)
		{
			Parts.Piece = 0;
		}
		else if (Parts.bGoal || Parts.Mechanism != 0)
		{
			Parts.bGoal = false;
			Parts.Mechanism = 0;
		}
		else
		{
			Parts = FCellParts();
		}
		break;
	}
	Cells[Index] = Encode(Parts);
	Mechanisms[Index] = EncodeMechanism(Parts);
	return Cells != Before || Mechanisms != MechanismsBefore;
}

bool FSokobanEditCanvas::Resize(const FIntPoint& NewSize)
{
	const FIntPoint Clamped = ClampSize(NewSize);
	if (Clamped == Size)
	{
		return false;
	}

	FSokobanEditCanvas Resized;
	Resized.Size = Clamped;
	Resized.Cells.Init(TEXT(' '), Clamped.X * Clamped.Y);
	Resized.Mechanisms.Init(TEXT(' '), Clamped.X * Clamped.Y);
	for (int32 Y = 0; Y < FMath::Min(Size.Y, Clamped.Y); ++Y)
	{
		for (int32 X = 0; X < FMath::Min(Size.X, Clamped.X); ++X)
		{
			Resized.Cells[Resized.IndexOf(FIntPoint(X, Y))] = Cells[IndexOf(FIntPoint(X, Y))];
			Resized.Mechanisms[Resized.IndexOf(FIntPoint(X, Y))] = Mechanisms[IndexOf(FIntPoint(X, Y))];
		}
	}
	*this = MoveTemp(Resized);
	return true;
}

bool FSokobanEditCanvas::WallIn()
{
	const TArray<TCHAR> Before = Cells;
	auto IsFloor = [&Before, this](const FIntPoint& Cell)
	{
		return IsInside(Cell) && Decode(Before[IndexOf(Cell)]).Base == ESokobanTile::Floor;
	};

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const FCellParts Parts = Decode(Before[IndexOf(Cell)], Mechanisms[IndexOf(Cell)]);
			if (Parts.Base == ESokobanTile::Void)
			{
				// Void next to floor (diagonals too, for closed corners) becomes wall.
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					for (int32 DX = -1; DX <= 1; ++DX)
					{
						if ((DX != 0 || DY != 0) && IsFloor(Cell + FIntPoint(DX, DY)))
						{
							Cells[IndexOf(Cell)] = TEXT('#');
						}
					}
				}
			}
			else if (Parts.Base == ESokobanTile::Floor && !Parts.bGoal && Parts.Piece == 0 && Parts.Mechanism == 0
				&& (X == 0 || Y == 0 || X == Size.X - 1 || Y == Size.Y - 1))
			{
				// Bare floor on the edge of the canvas has nowhere outside it for a wall: it becomes the wall.
				Cells[IndexOf(Cell)] = TEXT('#');
			}
		}
	}
	return Cells != Before;
}

TArray<FString> FSokobanEditCanvas::ToRows() const
{
	TArray<FString> Rows;
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		FString Row;
		for (int32 X = 0; X < Size.X; ++X)
		{
			Row.AppendChar(Cells[IndexOf(FIntPoint(X, Y))]);
		}
		Rows.Add(Row);
	}
	return Rows;
}

TArray<FString> FSokobanEditCanvas::ToMechanismRows() const
{
	TArray<FString> Rows;
	if (!Mechanisms.ContainsByPredicate([](TCHAR Mechanism) { return Mechanism != TEXT(' '); }))
	{
		return Rows;
	}
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		FString Row;
		for (int32 X = 0; X < Size.X; ++X)
		{
			Row.AppendChar(Mechanisms[IndexOf(FIntPoint(X, Y))]);
		}
		Rows.Add(Row.TrimEnd());
	}
	return Rows;
}

FSokobanLevel FSokobanEditCanvas::ToLevel(const FString& Id, const FString& Name, int32 Par) const
{
	FSokobanLevel Level;
	Level.Id = Id;
	Level.Name = Name;
	Level.Par = Par;
	Level.Layout.Size = Size;
	Level.Layout.Tiles.Init(ESokobanTile::Void, Size.X * Size.Y);
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const FCellParts Parts = Decode(Cells[IndexOf(Cell)], Mechanisms[IndexOf(Cell)]);
			Level.Layout.Tiles[IndexOf(Cell)] = Parts.Base;
			if (Parts.bGoal)
			{
				Level.Layout.Goals.Add(Cell);
			}
			if (Parts.Mechanism == TEXT('P'))
			{
				Level.Layout.Plates.Add(Cell);
			}
			else if (Parts.Mechanism == TEXT('B'))
			{
				Level.Layout.Barriers.Add(Cell);
			}
			if (Parts.Piece == TEXT('$'))
			{
				Level.InitialState.Crates.Add(Cell);
			}
			else if (Parts.Piece == TEXT('@'))
			{
				FSokobanHeroState& Hero = Level.InitialState.Heroes.AddDefaulted_GetRef();
				Hero.Cell = Cell;
				Hero.Facing = ESokobanDirection::South;
			}
		}
	}
	return Level;
}

TArray<ESokobanCanvasIssue> FSokobanEditCanvas::FindIssues() const
{
	TArray<ESokobanCanvasIssue> Issues;
	if (!HasHero())
	{
		Issues.Add(ESokobanCanvasIssue::NoHero);
	}
	const int32 Goals = CountGoals();
	if (Goals == 0)
	{
		Issues.Add(ESokobanCanvasIssue::NoGoal);
	}
	if (CountCrates() < Goals)
	{
		Issues.Add(ESokobanCanvasIssue::TooFewCrates);
	}
	if (Mechanisms.Contains(TEXT('B')) && !Mechanisms.Contains(TEXT('P')))
	{
		Issues.Add(ESokobanCanvasIssue::BarrierWithoutPlate);
	}

	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		if (Decode(Cells[Index]).Base != ESokobanTile::Floor)
		{
			continue;
		}
		const FIntPoint Cell(Index % Size.X, Index / Size.X);
		for (ESokobanDirection Direction : SokobanGrid::AllDirections)
		{
			const FIntPoint Next = Cell + SokobanGrid::Offset(Direction);
			if (!IsInside(Next) || Decode(Cells[IndexOf(Next)]).Base == ESokobanTile::Void)
			{
				Issues.Add(ESokobanCanvasIssue::NotEnclosed);
				return Issues;
			}
		}
	}
	return Issues;
}

int32 FSokobanEditCanvas::CountGoals() const
{
	int32 Count = 0;
	for (TCHAR Char : Cells)
	{
		Count += Decode(Char).bGoal ? 1 : 0;
	}
	return Count;
}

int32 FSokobanEditCanvas::CountCrates() const
{
	int32 Count = 0;
	for (TCHAR Char : Cells)
	{
		Count += Decode(Char).Piece == TEXT('$') ? 1 : 0;
	}
	return Count;
}

bool FSokobanEditCanvas::HasHero() const
{
	return Cells.Contains(TEXT('@')) || Cells.Contains(TEXT('+'));
}
