// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelEditor/SokobanLevelEditor.h"
#include "SokobanDemo.h"
#include "Game/SokobanAudioSubsystem.h"
#include "LevelEditor/SokobanLevelShelf.h"
#include "View/SokobanBoardView.h"
#include "View/SokobanCameraRig.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "SokobanLevelEditor"

namespace
{
	/** Room left on either side of the canvas for the editor's tool columns, in cells. */
	constexpr float ToolRoom = 2.5f;
}

ASokobanLevelEditor::ASokobanLevelEditor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASokobanLevelEditor::Init(ASokobanBoardView* InBoard, ASokobanCameraRig* InCamera)
{
	Board = InBoard;
	Camera = InCamera;
}

void ASokobanLevelEditor::Open(const FSokobanEditCanvas& InCanvas, const FString& InId, const FString& InName, bool bInSaved)
{
	Canvas = InCanvas;
	LevelId = InId;
	LevelName = InName;
	bStoredAsSaved = bInSaved;
	UndoStack.Reset();
	RedoStack.Reset();
	bStroking = false;
	bNameChanged = false;
	Revision = StoredRevision = 0;
	Verdict = ESokobanVerdict::Unknown;
	VerdictRevision = ProvenRevision = MessageRevision = -1;
	PendingSolve.Reset();
	bSaveWhenSolved = false;
	UE_LOG(LogSokoban, Log, TEXT("Editor: open %s \"%s\" (%dx%d)"), *LevelId, *LevelName, Canvas.GetSize().X, Canvas.GetSize().Y);
	Show();
}

void ASokobanLevelEditor::Close()
{
	Hide();
	Canvas = FSokobanEditCanvas();
	LevelId.Reset();
	LevelName.Reset();
	UndoStack.Reset();
	RedoStack.Reset();
	PendingSolve.Reset();
	bSaveWhenSolved = false;
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::Show()
{
	bShown = true;
	Board->SetCanvasOverlay(true);
	Redraw();
	FrameCamera();
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::Hide()
{
	bShown = false;
	bStroking = false;
	Board->SetCanvasOverlay(false);
}

void ASokobanLevelEditor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PendingSolve.IsValid() && PendingSolve.IsReady())
	{
		const FSokobanSolveResult Result = PendingSolve.Get();
		PendingSolve.Reset();
		if (PendingRevision == Revision)
		{
			FinishSolve(Result);
		}
	}

	if (!bShown)
	{
		return;
	}

	const TOptional<FIntPoint> Hovered = CellUnderMouse();
	Board->SetCanvasCursor(Hovered, bStroking ? bStrokeErase : Brush == ESokobanBrush::Erase);
	if (bStroking && Hovered.IsSet() && Hovered != LastStrokeCell)
	{
		LastStrokeCell = Hovered;
		if (Canvas.Paint(Hovered.GetValue(), bStrokeErase ? ESokobanBrush::Erase : Brush))
		{
			// A soft tick per painted cell, lower when erasing.
			if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
			{
				Audio->Play(ESokobanSound::Click, 0.3f, bStrokeErase ? 0.8f : 1.15f);
			}
			Changed(false);
		}
	}
}

void ASokobanLevelEditor::BeginStroke(bool bErase)
{
	if (!bShown || bStroking)
	{
		return;
	}
	bStroking = true;
	bStrokeErase = bErase;
	StrokeBefore = Canvas;
	LastStrokeCell.Reset();
}

void ASokobanLevelEditor::EndStroke()
{
	if (!bStroking)
	{
		return;
	}
	bStroking = false;
	if (Canvas != StrokeBefore)
	{
		UndoStack.Push(StrokeBefore);
		RedoStack.Reset();
		OnChanged.Broadcast();
	}
}

void ASokobanLevelEditor::SetBrush(ESokobanBrush InBrush)
{
	Brush = InBrush;
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return;
	}
	const bool bSizeChanged = UndoStack.Last().GetSize() != Canvas.GetSize();
	RedoStack.Push(Canvas);
	Canvas = UndoStack.Pop();
	Changed(bSizeChanged);
}

void ASokobanLevelEditor::Redo()
{
	if (RedoStack.Num() == 0)
	{
		return;
	}
	const bool bSizeChanged = RedoStack.Last().GetSize() != Canvas.GetSize();
	UndoStack.Push(Canvas);
	Canvas = RedoStack.Pop();
	Changed(bSizeChanged);
}

void ASokobanLevelEditor::Resize(const FIntPoint& Delta)
{
	ApplyChange([&Delta](FSokobanEditCanvas& Target) { return Target.Resize(Target.GetSize() + Delta); });
}

void ASokobanLevelEditor::WallIn()
{
	ApplyChange([](FSokobanEditCanvas& Target) { return Target.WallIn(); });
}

void ASokobanLevelEditor::PaintRect(const FIntPoint& From, const FIntPoint& To, ESokobanBrush InBrush)
{
	ApplyChange([&](FSokobanEditCanvas& Target)
	{
		bool bAny = false;
		for (int32 Y = FMath::Min(From.Y, To.Y); Y <= FMath::Max(From.Y, To.Y); ++Y)
		{
			for (int32 X = FMath::Min(From.X, To.X); X <= FMath::Max(From.X, To.X); ++X)
			{
				bAny |= Target.Paint(FIntPoint(X, Y), InBrush);
			}
		}
		return bAny;
	});
}

void ASokobanLevelEditor::ApplyChange(TFunctionRef<bool(FSokobanEditCanvas&)> Change)
{
	FSokobanEditCanvas Before = Canvas;
	if (Change(Canvas))
	{
		const bool bSizeChanged = Before.GetSize() != Canvas.GetSize();
		UndoStack.Push(MoveTemp(Before));
		RedoStack.Reset();
		Changed(bSizeChanged);
	}
}

void ASokobanLevelEditor::Changed(bool bSizeChanged)
{
	++Revision;
	bSaveWhenSolved = false;
	Redraw();
	if (bSizeChanged)
	{
		FrameCamera();
	}
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::SetLevelName(const FString& Name)
{
	const FString Trimmed = Name.TrimStartAndEnd();
	if (!Trimmed.IsEmpty() && Trimmed != LevelName)
	{
		// A new name is worth storing too, but it doesn't change what the solver found.
		LevelName = Trimmed;
		bNameChanged = true;
		OnChanged.Broadcast();
	}
}

void ASokobanLevelEditor::Redraw()
{
	if (bShown)
	{
		Board->BuildLevel(Canvas.ToLevel(LevelId, LevelName, 0));
	}
}

void ASokobanLevelEditor::FrameCamera()
{
	// Leave room beside the canvas for the tool columns on either side of the screen.
	FBox Bounds = Board->GetBoundsForSize(Canvas.GetSize());
	Bounds = Bounds.ExpandBy(FVector(0.f, ToolRoom * Board->CellSize, 0.f));
	Camera->FrameBounds(Bounds, Bounds, false);
}

TOptional<FIntPoint> ASokobanLevelEditor::CellUnderMouse() const
{
	const APlayerController* Player = GetWorld()->GetFirstPlayerController();
	float X = 0.f;
	float Y = 0.f;
	if (!Player || !Player->GetMousePosition(X, Y))
	{
		return {};
	}
	return CellAtScreen(FVector2D(X, Y));
}

TOptional<FIntPoint> ASokobanLevelEditor::CellAtScreen(const FVector2D& ScreenPosition) const
{
	const APlayerController* Player = GetWorld()->GetFirstPlayerController();
	FVector Start;
	FVector Direction;
	if (!Player || !Player->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, Start, Direction) || Direction.Z > -KINDA_SMALL_NUMBER)
	{
		return {};
	}

	// March down the ray from above the tallest walls to the floor, and take the first cell whose contents it
	// passes through: the top of a wall or a crate in front counts, not just the floor behind it.
	const float FloorZ = Board->GetActorLocation().Z;
	const float Near = (FloorZ + 220.f - Start.Z) / Direction.Z;
	const float Far = (FloorZ - Start.Z) / Direction.Z;
	for (float T = Near; T < Far; T += 5.f)
	{
		const FVector Point = Start + Direction * T;
		const FIntPoint Cell = Board->WorldToCell(Point);
		if (Canvas.IsInside(Cell) && Point.Z - FloorZ <= CellTop(Cell))
		{
			return Cell;
		}
	}
	const FIntPoint FloorCell = Board->WorldToCell(Start + Direction * Far);
	return Canvas.IsInside(FloorCell) ? TOptional<FIntPoint>(FloorCell) : TOptional<FIntPoint>();
}

float ASokobanLevelEditor::CellTop(const FIntPoint& Cell) const
{
	switch (Canvas.GetCell(Cell))
	{
	case TEXT('$'):
	case TEXT('*'):
		return 75.f;
	case TEXT('@'):
	case TEXT('+'):
		return 85.f;
	default:
		return Board->WallTop(Cell);
	}
}

void ASokobanLevelEditor::CheckSolvable()
{
	if (Canvas.FindIssues().Num() > 0)
	{
		SetMessage(FText::Format(LOCTEXT("CantCheck", "还不能检查：{0}"), DescribeIssue(Canvas.FindIssues()[0], Canvas)), true);
		return;
	}
	if (PendingSolve.IsValid() && PendingRevision == Revision)
	{
		return;
	}

	// The search can take a moment on bigger levels: run it off the game thread and pick the result up in Tick.
	const FSokobanLevel Level = Canvas.ToLevel(LevelId, LevelName, 0);
	PendingSolve = Async(EAsyncExecution::ThreadPool, [Layout = Level.Layout, Start = Level.InitialState]()
	{
		return FSokobanSolver::Solve(Layout, Start);
	});
	PendingRevision = Revision;
	Verdict = ESokobanVerdict::Checking;
	VerdictRevision = Revision;
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::FinishSolve(const FSokobanSolveResult& Result)
{
	Verdict = Result.bSolved ? ESokobanVerdict::Solvable : (Result.bProvenUnsolvable ? ESokobanVerdict::Unsolvable : ESokobanVerdict::TooHard);
	VerdictMoves = Result.Moves.Num();
	VerdictRevision = Revision;
	UE_LOG(LogSokoban, Log, TEXT("Editor: %s is %s (%d moves, %d states)"), *LevelId,
		Result.bSolved ? TEXT("solvable") : (Result.bProvenUnsolvable ? TEXT("unsolvable") : TEXT("undecided")), VerdictMoves, Result.ExploredStates);

	if (bSaveWhenSolved)
	{
		bSaveWhenSolved = false;
		SaveLevel();
	}
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::SaveLevel()
{
	const TArray<ESokobanCanvasIssue> Issues = Canvas.FindIssues();
	if (Issues.Num() > 0)
	{
		SetMessage(FText::Format(LOCTEXT("CantSave", "还不能保存：{0}"), DescribeIssue(Issues[0], Canvas)), true);
		return;
	}

	const bool bVerdictCurrent = VerdictRevision == Revision;
	if (bVerdictCurrent && Verdict == ESokobanVerdict::Solvable)
	{
		StoreSaved(ProvenRevision == Revision ? FMath::Min(VerdictMoves, ProvenMoves) : VerdictMoves);
	}
	else if (ProvenRevision == Revision)
	{
		StoreSaved(ProvenMoves);
	}
	else if (bVerdictCurrent && Verdict == ESokobanVerdict::Unsolvable)
	{
		SetMessage(LOCTEXT("SaveUnsolvable", "这一关解不开，不能保存为关卡；可以先存为草稿"), true);
	}
	else if (bVerdictCurrent && Verdict == ESokobanVerdict::TooHard)
	{
		SetMessage(LOCTEXT("SaveTooHard", "关卡太复杂，没能算出是否可解：试玩通关一次就能保存"), true);
	}
	else
	{
		// Find out first; FinishSolve comes back here.
		bSaveWhenSolved = true;
		CheckSolvable();
	}
}

void ASokobanLevelEditor::StoreSaved(int32 Par)
{
	FString Error;
	const FSokobanLevelData Data = Canvas.ToLevel(LevelId, LevelName, Par).ToData();
	if (!SokobanShelf::Store(SokobanShelf::GetDir(ESokobanShelf::Saved), Data, Error))
	{
		UE_LOG(LogSokoban, Error, TEXT("Editor: saving %s failed: %s"), *LevelId, *Error);
		SetMessage(LOCTEXT("SaveFailed", "保存失败，请看日志"), true);
		return;
	}
	// The finished level replaces its draft.
	SokobanShelf::Remove(SokobanShelf::GetDir(ESokobanShelf::Drafts), LevelId);
	bStoredAsSaved = true;
	StoredRevision = Revision;
	bNameChanged = false;
	UE_LOG(LogSokoban, Log, TEXT("Editor: saved %s \"%s\", par %d"), *LevelId, *LevelName, Par);
	SetMessage(FText::Format(LOCTEXT("Saved", "已保存到「保存的关卡」，满星 {0} 步"), Par), false);
}

bool ASokobanLevelEditor::StoreDraft()
{
	FSokobanLevelData Data;
	Data.Id = LevelId;
	Data.Name = LevelName;
	Data.Map = Canvas.ToRows();
	Data.Mechanisms = Canvas.ToMechanismRows();
	FString Error;
	if (!SokobanShelf::Store(SokobanShelf::GetDir(ESokobanShelf::Drafts), Data, Error))
	{
		UE_LOG(LogSokoban, Error, TEXT("Editor: storing draft %s failed: %s"), *LevelId, *Error);
		return false;
	}
	StoredRevision = Revision;
	bNameChanged = false;
	UE_LOG(LogSokoban, Log, TEXT("Editor: draft %s \"%s\" stored"), *LevelId, *LevelName);
	return true;
}

void ASokobanLevelEditor::SaveDraft()
{
	const bool bStored = StoreDraft();
	SetMessage(bStored ? LOCTEXT("DraftSaved", "已存为草稿") : LOCTEXT("DraftFailed", "存草稿失败，请看日志"), !bStored);
}

void ASokobanLevelEditor::SaveDraftIfChanged()
{
	if (HasSession() && (Revision != StoredRevision || bNameChanged))
	{
		StoreDraft();
	}
}

bool ASokobanLevelEditor::CanTestPlay() const
{
	return Canvas.FindIssues().Num() == 0;
}

FSokobanLevel ASokobanLevelEditor::MakeTestLevel() const
{
	const bool bKnown = VerdictRevision == Revision && Verdict == ESokobanVerdict::Solvable;
	return Canvas.ToLevel(LevelId, LevelName, bKnown ? VerdictMoves : 0);
}

void ASokobanLevelEditor::NoteTestPlaySolved(int32 Moves)
{
	if (ProvenRevision != Revision || Moves < ProvenMoves)
	{
		ProvenMoves = Moves;
	}
	ProvenRevision = Revision;
	OnChanged.Broadcast();
}

void ASokobanLevelEditor::SetMessage(const FText& InMessage, bool bProblem)
{
	Message = InMessage;
	bMessageProblem = bProblem;
	MessageRevision = Revision;
	OnChanged.Broadcast();
}

FText ASokobanLevelEditor::DescribeIssue(ESokobanCanvasIssue Issue, const FSokobanEditCanvas& Canvas)
{
	switch (Issue)
	{
	case ESokobanCanvasIssue::NoHero:
		return LOCTEXT("NoHero", "还没有放大橘（笔刷 5）");
	case ESokobanCanvasIssue::NoGoal:
		return LOCTEXT("NoGoal", "还没有目标点（笔刷 4）");
	case ESokobanCanvasIssue::TrapdoorWithoutPlate:
		return LOCTEXT("NoPlate", "有活板门但还没有机关（笔刷 6）");
	case ESokobanCanvasIssue::TooFewCrates:
		return FText::Format(LOCTEXT("TooFewCrates", "目标点比箱子多：{0} 个目标点，{1} 个箱子"), Canvas.CountGoals(), Canvas.CountCrates());
	default:
		return LOCTEXT("NotEnclosed", "地面外围没有用墙围住（可以点「自动围墙」）");
	}
}

FText ASokobanLevelEditor::GetStatus() const
{
	if (MessageRevision == Revision && !Message.IsEmpty())
	{
		return Message;
	}

	const TArray<ESokobanCanvasIssue> Issues = Canvas.FindIssues();
	if (Issues.Num() > 0)
	{
		return DescribeIssue(Issues[0], Canvas);
	}
	if (VerdictRevision == Revision)
	{
		switch (Verdict)
		{
		case ESokobanVerdict::Checking:
			return LOCTEXT("Checking", "正在计算能不能解开……");
		case ESokobanVerdict::Solvable:
			return FText::Format(LOCTEXT("Solvable", "可以解开：最少 {0} 步，就是这一关的满星步数"), VerdictMoves);
		case ESokobanVerdict::Unsolvable:
			return LOCTEXT("Unsolvable", "这样摆解不开，调整一下箱子或目标点吧");
		case ESokobanVerdict::TooHard:
			return LOCTEXT("TooHard", "太复杂了，没能算完；试玩通关一次也能证明可解");
		default:
			break;
		}
	}
	if (ProvenRevision == Revision)
	{
		return FText::Format(LOCTEXT("Proven", "试玩用 {0} 步通关，可以保存了"), ProvenMoves);
	}
	return LOCTEXT("Ready", "可以试玩了；保存时会先检查能不能解开");
}

bool ASokobanLevelEditor::IsStatusProblem() const
{
	if (MessageRevision == Revision && !Message.IsEmpty())
	{
		return bMessageProblem;
	}
	if (Canvas.FindIssues().Num() > 0)
	{
		return true;
	}
	return VerdictRevision == Revision && (Verdict == ESokobanVerdict::Unsolvable || Verdict == ESokobanVerdict::TooHard);
}

#undef LOCTEXT_NAMESPACE
