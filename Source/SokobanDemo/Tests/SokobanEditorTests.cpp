// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "LevelEditor/SokobanLevelShelf.h"
#include "Misc/Paths.h"
#include "Puzzle/SokobanEditCanvas.h"
#include "Puzzle/SokobanLevel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SokobanEditorTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FString Row(const FSokobanEditCanvas& Canvas, int32 Y)
	{
		return Canvas.ToRows()[Y];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanEditorBrushTest, "Sokoban.Editor.Brushes", SokobanEditorTest::Flags)
bool FSokobanEditorBrushTest::RunTest(const FString& Parameters)
{
	FSokobanEditCanvas Canvas = FSokobanEditCanvas::MakeWalledRoom(FIntPoint(6, 4));
	TestEqual(TEXT("A new room is floor ringed by walls"), SokobanEditorTest::Row(Canvas, 1), FString(TEXT("#----#")));

	Canvas.Paint(FIntPoint(2, 1), ESokobanBrush::Goal);
	Canvas.Paint(FIntPoint(2, 1), ESokobanBrush::Crate);
	TestEqual(TEXT("A crate painted on a goal stands on it"), Canvas.GetCell(FIntPoint(2, 1)), TEXT('*'));
	Canvas.Paint(FIntPoint(2, 1), ESokobanBrush::Erase);
	TestEqual(TEXT("Erasing takes the crate first"), Canvas.GetCell(FIntPoint(2, 1)), TEXT('.'));
	Canvas.Paint(FIntPoint(2, 1), ESokobanBrush::Erase);
	TestEqual(TEXT("then the goal"), Canvas.GetCell(FIntPoint(2, 1)), TEXT('-'));
	Canvas.Paint(FIntPoint(2, 1), ESokobanBrush::Erase);
	TestEqual(TEXT("then the floor itself"), Canvas.GetCell(FIntPoint(2, 1)), TEXT(' '));

	Canvas.Paint(FIntPoint(1, 1), ESokobanBrush::Hero);
	Canvas.Paint(FIntPoint(3, 2), ESokobanBrush::Goal);
	Canvas.Paint(FIntPoint(3, 2), ESokobanBrush::Hero);
	TestEqual(TEXT("Placing the hero again moves it"), Canvas.GetCell(FIntPoint(1, 1)), TEXT('-'));
	TestEqual(TEXT("The hero can stand on a goal"), Canvas.GetCell(FIntPoint(3, 2)), TEXT('+'));

	TestTrue(TEXT("A wall replaces whatever was there"), Canvas.Paint(FIntPoint(3, 2), ESokobanBrush::Wall));
	TestEqual(TEXT("...hero and goal included"), Canvas.GetCell(FIntPoint(3, 2)), TEXT('#'));
	TestFalse(TEXT("Painting the same thing again changes nothing"), Canvas.Paint(FIntPoint(3, 2), ESokobanBrush::Wall));
	TestFalse(TEXT("Painting outside the canvas changes nothing"), Canvas.Paint(FIntPoint(9, 9), ESokobanBrush::Floor));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanEditorMechanismTest, "Sokoban.Editor.Mechanisms", SokobanEditorTest::Flags)
bool FSokobanEditorMechanismTest::RunTest(const FString& Parameters)
{
	FSokobanEditCanvas Canvas = FSokobanEditCanvas::MakeWalledRoom(FIntPoint(7, 4));
	Canvas.Paint(FIntPoint(1, 1), ESokobanBrush::Hero);
	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Crate);
	Canvas.Paint(FIntPoint(5, 1), ESokobanBrush::Goal);
	Canvas.Paint(FIntPoint(4, 1), ESokobanBrush::Trapdoor);
	TestEqual(TEXT("A trapdoor needs a plate"), Canvas.FindIssues(), TArray<ESokobanCanvasIssue>{ ESokobanCanvasIssue::TrapdoorWithoutPlate });

	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Plate);
	TestEqual(TEXT("A plate goes under the crate"), Canvas.GetCell(FIntPoint(2, 2)), TEXT('$'));
	TestEqual(TEXT("...as a mechanism"), Canvas.GetMechanism(FIntPoint(2, 2)), TEXT('P'));
	TestEqual(TEXT("With a plate the drawing can be played"), Canvas.FindIssues().Num(), 0);

	Canvas.Paint(FIntPoint(4, 1), ESokobanBrush::Goal);
	TestEqual(TEXT("A goal replaces the trapdoor"), Canvas.GetMechanism(FIntPoint(4, 1)), TEXT(' '));
	Canvas.Paint(FIntPoint(4, 1), ESokobanBrush::Trapdoor);
	TestEqual(TEXT("...and a trapdoor the goal"), Canvas.GetCell(FIntPoint(4, 1)), TEXT('-'));

	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Erase);
	TestEqual(TEXT("Erasing takes the crate off the plate first"), Canvas.GetMechanism(FIntPoint(2, 2)), TEXT('P'));
	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Erase);
	TestTrue(TEXT("then the plate"), Canvas.GetMechanism(FIntPoint(2, 2)) == TEXT(' ') && Canvas.GetCell(FIntPoint(2, 2)) == TEXT('-'));
	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Plate);
	Canvas.Paint(FIntPoint(3, 2), ESokobanBrush::Crate);

	// Drafts and level files both keep the mechanisms.
	const FSokobanEditCanvas Draft = FSokobanEditCanvas::FromRows(Canvas.ToRows(), Canvas.ToMechanismRows());
	TestTrue(TEXT("Draft rows read back unchanged"), Draft == Canvas);
	FSokobanLevel Parsed;
	FString Error;
	TestTrue(FString::Printf(TEXT("The level file parses (%s)"), *Error), FSokobanLevel::FromData(Canvas.ToLevel(TEXT("m"), TEXT("M"), 0).ToData(), Parsed, Error));
	TestTrue(TEXT("Reading it back gives the same drawing"), FSokobanEditCanvas::FromLevel(Parsed) == Canvas);
	TestTrue(TEXT("Wall brush clears the mechanism"), Canvas.Paint(FIntPoint(4, 1), ESokobanBrush::Wall) && Canvas.GetMechanism(FIntPoint(4, 1)) == TEXT(' '));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanEditorIssuesTest, "Sokoban.Editor.Issues", SokobanEditorTest::Flags)
bool FSokobanEditorIssuesTest::RunTest(const FString& Parameters)
{
	FSokobanEditCanvas Canvas = FSokobanEditCanvas::MakeWalledRoom(FIntPoint(6, 5));
	TestTrue(TEXT("An empty room has no hero"), Canvas.FindIssues().Contains(ESokobanCanvasIssue::NoHero));
	TestTrue(TEXT("...and no goal"), Canvas.FindIssues().Contains(ESokobanCanvasIssue::NoGoal));

	Canvas.Paint(FIntPoint(1, 1), ESokobanBrush::Hero);
	Canvas.Paint(FIntPoint(4, 3), ESokobanBrush::Goal);
	TestEqual(TEXT("A goal without a crate"), Canvas.FindIssues(), TArray<ESokobanCanvasIssue>{ ESokobanCanvasIssue::TooFewCrates });
	Canvas.Paint(FIntPoint(2, 2), ESokobanBrush::Crate);
	TestEqual(TEXT("Hero, goal and crate in a walled room can be played"), Canvas.FindIssues().Num(), 0);

	Canvas.Paint(FIntPoint(5, 2), ESokobanBrush::Erase);
	TestTrue(TEXT("A gap in the wall leaves the floor open"), Canvas.FindIssues().Contains(ESokobanCanvasIssue::NotEnclosed));
	TestTrue(TEXT("Walling in changes the canvas"), Canvas.WallIn());
	TestEqual(TEXT("...and closes the gap"), Canvas.FindIssues().Num(), 0);

	FSokobanEditCanvas Edge = FSokobanEditCanvas::FromRows({ TEXT("-@$.") });
	TestTrue(TEXT("Floor on the canvas edge is open"), Edge.FindIssues().Contains(ESokobanCanvasIssue::NotEnclosed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanEditorRoundTripTest, "Sokoban.Editor.RoundTrip", SokobanEditorTest::Flags)
bool FSokobanEditorRoundTripTest::RunTest(const FString& Parameters)
{
	// A playable drawing survives the trip through the level file format.
	FSokobanEditCanvas Canvas = FSokobanEditCanvas::FromRows({
		TEXT("  #####   "),
		TEXT("###---####"),
		TEXT("#-@$-*--.#"),
		TEXT("#--------#"),
		TEXT("##########") });
	TestEqual(TEXT("The drawing can be played"), Canvas.FindIssues().Num(), 0);

	FSokobanLevel Parsed;
	FString Error;
	const FSokobanLevelData Data = Canvas.ToLevel(TEXT("rt"), TEXT("Round trip"), 7).ToData();
	TestTrue(FString::Printf(TEXT("The level file parses (%s)"), *Error), FSokobanLevel::FromData(Data, Parsed, Error));
	TestTrue(TEXT("Reading it back gives the same drawing"), FSokobanEditCanvas::FromLevel(Parsed) == Canvas);
	TestEqual(TEXT("Par is kept"), Parsed.Par, 7);

	// A draft keeps floor next to the void exactly as drawn.
	const FSokobanEditCanvas Draft = FSokobanEditCanvas::FromRows({ TEXT("  --  "), TEXT(" #@-$ "), TEXT("      ") });
	TestTrue(TEXT("Draft rows read back unchanged"), FSokobanEditCanvas::FromRows(Draft.ToRows()) == Draft);

	FSokobanEditCanvas Resized = Canvas;
	TestTrue(TEXT("Growing changes the size"), Resized.Resize(FIntPoint(12, 6)));
	TestEqual(TEXT("...keeping the top-left corner"), Resized.GetCell(FIntPoint(2, 2)), TEXT('@'));
	TestEqual(TEXT("...with void added"), Resized.GetCell(FIntPoint(11, 5)), TEXT(' '));
	Resized.Resize(FIntPoint(40, 40));
	TestEqual(TEXT("Size is capped"), Resized.GetSize(), FSokobanEditCanvas::MaxSize);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSokobanEditorShelfTest, "Sokoban.Editor.Shelf", SokobanEditorTest::Flags)
bool FSokobanEditorShelfTest::RunTest(const FString& Parameters)
{
	const FString Dir = FPaths::ProjectIntermediateDir() / TEXT("SokobanShelfTest");
	IFileManager::Get().DeleteDirectory(*Dir, false, true);

	FSokobanLevelData First;
	First.Id = SokobanShelf::MakeId();
	First.Name = TEXT("First");
	First.Map = { TEXT("#####"), TEXT("#@$.#"), TEXT("#####") };
	FSokobanLevelData Second = First;
	Second.Id = SokobanShelf::MakeId();
	Second.Name = TEXT("Second");
	TestNotEqual(TEXT("Ids are unique"), First.Id, Second.Id);

	FString Error;
	TestTrue(TEXT("Storing works"), SokobanShelf::Store(Dir, First, Error) && SokobanShelf::Store(Dir, Second, Error));
	TArray<FSokobanShelfEntry> Entries = SokobanShelf::List(Dir);
	TestEqual(TEXT("Both levels are listed"), Entries.Num(), 2);
	TestTrue(TEXT("Rows come back as stored"), Entries.ContainsByPredicate([&First](const FSokobanShelfEntry& Entry) { return Entry.Data.Map == First.Map; }));

	TestTrue(TEXT("Removing works"), SokobanShelf::Remove(Dir, First.Id));
	Entries = SokobanShelf::List(Dir);
	TestTrue(TEXT("Only the other level is left"), Entries.Num() == 1 && Entries[0].Data.Id == Second.Id);

	IFileManager::Get().DeleteDirectory(*Dir, false, true);
	return true;
}

#endif
