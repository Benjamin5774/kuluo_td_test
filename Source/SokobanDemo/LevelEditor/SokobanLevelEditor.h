// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "GameFramework/Actor.h"
#include "Puzzle/SokobanEditCanvas.h"
#include "Puzzle/SokobanSolver.h"
#include "SokobanLevelEditor.generated.h"

class ASokobanBoardView;
class ASokobanCameraRig;

/** What the editor knows about whether the drawing can be solved. */
enum class ESokobanVerdict : uint8
{
	Unknown,
	Checking,
	Solvable,
	Unsolvable,
	/** The solver gave up before deciding. */
	TooHard
};

/**
 * The in-game level editor's working session: the drawing, its undo history, the brush, the level's name and
 * id, what the solver found, and whether it is stored. While shown it draws the drawing on the board as the
 * castle it would become, with plates over empty cells and a highlight on the cell under the mouse. The player
 * controller feeds it brush strokes; the editor screen shows its state and calls its actions.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanLevelEditor : public AActor
{
	GENERATED_BODY()

public:
	ASokobanLevelEditor();

	virtual void Tick(float DeltaSeconds) override;

	/** The board and camera the editor draws with. */
	void Init(ASokobanBoardView* InBoard, ASokobanCameraRig* InCamera);

	/** Starts a session on Canvas, stored as Id under Name; bInSaved when it is already a saved level. */
	void Open(const FSokobanEditCanvas& InCanvas, const FString& InId, const FString& InName, bool bInSaved);
	bool HasSession() const { return !LevelId.IsEmpty(); }

	/** Ends the session without storing anything. */
	void Close();

	/** Draws the session on the board (entering the editor, coming back from a test play). */
	void Show();

	/** Stops drawing on the board; the session stays. */
	void Hide();

	/** Paints with the brush (or erases) under the mouse for as long as the stroke lasts: one undo step. */
	void BeginStroke(bool bErase);
	void EndStroke();

	void SetBrush(ESokobanBrush InBrush);
	ESokobanBrush GetBrush() const { return Brush; }

	bool CanUndo() const { return UndoStack.Num() > 0; }
	bool CanRedo() const { return RedoStack.Num() > 0; }
	void Undo();
	void Redo();

	/** Grows or shrinks the canvas by Delta columns and rows. */
	void Resize(const FIntPoint& Delta);

	/** Rings the floor with walls wherever it is open. */
	void WallIn();

	/** Dev: paints the rectangle between two corners with Brush, as one undo step. */
	void PaintRect(const FIntPoint& From, const FIntPoint& To, ESokobanBrush InBrush);

	/**
	 * The canvas cell drawn at a point of the screen (viewport pixels). Walls and pieces standing in the way
	 * count: pointing at the top of a crate picks the crate's cell, not the floor behind it.
	 */
	TOptional<FIntPoint> CellAtScreen(const FVector2D& ScreenPosition) const;

	void SetLevelName(const FString& Name);
	const FString& GetLevelName() const { return LevelName; }
	const FString& GetLevelId() const { return LevelId; }
	const FSokobanEditCanvas& GetCanvas() const { return Canvas; }

	/** Runs the solver in the background. */
	void CheckSolvable();

	/** Saves as a finished level once it can be played and is proven solvable (checking first if need be). */
	void SaveLevel();

	/** Stores the drawing as it is on the drafts shelf. */
	void SaveDraft();

	/** Quietly stores a draft if anything changed since the drawing was last stored. */
	void SaveDraftIfChanged();

	/** Whether the drawing can be played (no issues), and the level to play. */
	bool CanTestPlay() const;
	FSokobanLevel MakeTestLevel() const;

	/** A test play of the current drawing was solved in Moves: that proves it solvable. */
	void NoteTestPlaySolved(int32 Moves);

	/** One line for the editor screen: the last action's result, the first problem, or what the solver found. */
	FText GetStatus() const;
	bool IsStatusProblem() const;

	/** Broadcast whenever anything the editor screen shows changes. */
	FSimpleMulticastDelegate OnChanged;

	/** How a problem with the drawing reads on screen. */
	static FText DescribeIssue(ESokobanCanvasIssue Issue, const FSokobanEditCanvas& Canvas);

private:
	/** Applies Change; when it changes the canvas, records an undo step and redraws. */
	void ApplyChange(TFunctionRef<bool(FSokobanEditCanvas&)> Change);

	/** The canvas changed: invalidate verdicts and messages, redraw, tell the screen. */
	void Changed(bool bSizeChanged);

	void Redraw();
	void FrameCamera();

	/** The canvas cell the mouse points at (see CellAtScreen). */
	TOptional<FIntPoint> CellUnderMouse() const;

	/** Height of whatever stands in Cell, for picking. */
	float CellTop(const FIntPoint& Cell) const;

	void FinishSolve(const FSokobanSolveResult& Result);
	void StoreSaved(int32 Par);
	bool StoreDraft();
	void SetMessage(const FText& Message, bool bProblem);

	UPROPERTY(Transient)
	TObjectPtr<ASokobanBoardView> Board;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanCameraRig> Camera;

	FSokobanEditCanvas Canvas;
	FString LevelId;
	FString LevelName;
	bool bStoredAsSaved = false;
	bool bShown = false;

	/** Renamed since the level was last stored. */
	bool bNameChanged = false;

	TArray<FSokobanEditCanvas> UndoStack;
	TArray<FSokobanEditCanvas> RedoStack;

	ESokobanBrush Brush = ESokobanBrush::Wall;
	bool bStroking = false;
	bool bStrokeErase = false;
	FSokobanEditCanvas StrokeBefore;
	TOptional<FIntPoint> LastStrokeCell;

	/** Counts changes to the canvas; verdicts, proofs and messages belong to the revision they were made at. */
	int32 Revision = 0;
	int32 StoredRevision = 0;

	ESokobanVerdict Verdict = ESokobanVerdict::Unknown;
	int32 VerdictRevision = -1;
	int32 VerdictMoves = 0;

	TFuture<FSokobanSolveResult> PendingSolve;
	int32 PendingRevision = -1;
	bool bSaveWhenSolved = false;

	int32 ProvenRevision = -1;
	int32 ProvenMoves = 0;

	FText Message;
	bool bMessageProblem = false;
	int32 MessageRevision = -1;
};
