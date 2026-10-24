// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Puzzle/SokobanRules.h"
#include "Puzzle/SokobanTypes.h"
#include "SokobanPlayerController.generated.h"

class ASokobanGameMode;
class ASokobanLevelEditor;
class UInputAction;
class UInputMappingContext;
class USokobanEditorWidget;
class USokobanHUDWidget;
class USokobanLevelSelectWidget;
class USokobanMenuPanel;
class USokobanShelfWidget;
class UUserWidget;
enum class ESokobanFlowState : uint8;
struct FInputActionValue;

/**
 * Turns input into puzzle moves and level editor strokes, and drives the UI.
 * Moves: one cell per press and steady repeats while held. A move always plays out: a press during it is
 * kept and played the moment it lands, so steps join into one continuous walk. Undo rewinds at once, and
 * holding it rewinds faster and faster. In the editor the mouse paints (left) and erases (right), 1-6 pick a
 * brush, Z / Y undo and redo. Screens follow the game mode's flow state.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASokobanPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Dev: plays back the shortest solution of the current level, starting after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanSolve(float Delay = 0.f);

	/** Dev: saves Saved/Screenshots/<Name>.png (with UI) after Delay seconds, then quits when bQuit. */
	UFUNCTION(Exec)
	void SokobanShot(float Delay = 1.f, bool bQuit = false, const FString& Name = TEXT("SokobanShot"));

	/** Dev: starts a level (0-based) after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanLevel(int32 Index, float Delay = 0.f);

	/**
	 * Dev: after Delay seconds, opens a screen: menu, levels, pause, editor, saved, drafts, new (a new drawing),
	 * continue (the drawing to continue) or testsaved (test-plays the newest saved level).
	 */
	UFUNCTION(Exec)
	void SokobanScreen(const FString& Screen, float Delay = 0.f);

	/** Dev: clicks the Index-th button (0-based) of the menu or level list on screen after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanPick(int32 Index, float Delay = 0.f);

	/** Dev: forgets all saved progress. */
	UFUNCTION(Exec)
	void SokobanResetProgress();

	/** Dev: logs the board pieces after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanDump(float Delay = 0.f);

	/** Dev: zooms the camera by Steps wheel notches (positive in) after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanZoom(float Steps, float Delay = 0.f);

	/** Dev: presses (bDown) or releases a direction key, N, S, E or W, after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanKey(const FString& Direction, bool bDown, float Delay = 0.f);

	/**
	 * Dev: in the editor, paints the rectangle from (X, Y) to (X2, Y2) (or just the cell when X2 < 0) with Brush
	 * (floor, wall, crate, goal, hero or erase) after Delay seconds.
	 */
	UFUNCTION(Exec)
	void SokobanEdit(const FString& Brush, int32 X, int32 Y, int32 X2 = -1, int32 Y2 = -1, float Delay = 0.f);

	/** Dev: in the editor, paints with Brush the cell drawn at screen point (X, Y), as a click there would, after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanEditAt(const FString& Brush, float X, float Y, float Delay = 0.f);

	/**
	 * Dev: in the editor, after Delay seconds: undo, redo, wallin, check, test, save, draft, back, wider,
	 * narrower, taller or shorter.
	 */
	UFUNCTION(Exec)
	void SokobanEditAction(const FString& Action, float Delay = 0.f);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanHUDWidget> HUDClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanMenuPanel> MenuPanelClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanLevelSelectWidget> LevelSelectClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanEditorWidget> EditorScreenClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanShelfWidget> ShelfScreenClass;

	/** Seconds a direction must be held before it starts repeating (it repeats when the current move lands). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float MoveRepeatDelay = 0.12f;

	/**
	 * A turn pressed within this many seconds after a held key carried the hero on into another step takes
	 * that step back and turns at the cell the hero was leaving, so a turn a moment late doesn't overshoot.
	 */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float TurnGrace = 0.09f;

	/** Seconds the undo key must be held before rewinding continuously. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float UndoRepeatDelay = 0.35f;

	/** First interval of the continuous rewind; it then speeds up down to UndoRepeatMinInterval. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float UndoRepeatInterval = 0.14f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float UndoRepeatMinInterval = 0.04f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float UndoRepeatAcceleration = 0.85f;

private:
	void CreateInputMappings();
	void CreateWidgets();

	void OnDirectionPressed(ESokobanDirection Direction);
	void OnDirectionReleased(ESokobanDirection Direction);
	void OnUndoPressed();
	void OnUndoReleased();
	void OnRestartPressed();
	void OnPausePressed();
	void OnZoom(const FInputActionValue& Value);
	void OnStrokePressed(bool bErase);
	void OnStrokeReleased(bool bErase);
	void OnRedoPressed();
	void OnBrushPressed(int32 Brush);

	/** The level editor, while the player is drawing (and not typing the level's name); null otherwise. */
	ASokobanLevelEditor* GetDrawingEditor() const;

	/** Sends a move to the game mode; the next one waits until its animation has played out. */
	FSokobanMoveResult IssueMove(ESokobanDirection Direction);

	/** Turns at the cell the hero is leaving if a held key just carried it on (see TurnGrace). */
	bool TryTurnLate(ESokobanDirection Direction);

	/** Whether the last move, undo or restart is still animating. */
	bool IsBoardBusy() const;

	/** Notes how long the board animates after a request to the game mode. */
	void MarkBoardBusy();

	bool IsPlaying() const;
	ASokobanGameMode* GetSokobanGameMode() const;

	void HandleFlowStateChanged(ESokobanFlowState State);
	void HandlePuzzleChanged();
	void PresentMainMenu();
	void PresentLevelSelect();
	void PresentPauseMenu();
	void PresentResults();
	void PresentEditorHome();
	void PresentShelf();
	void PresentDrawing();

	/** A yes / no question on the menu panel; OnCancel also runs on Esc. */
	void PresentConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, TFunction<void()> OnConfirm, TFunction<void()> OnCancel);

	/** Routes keyboard / gamepad input to the UI and shows the cursor; the screen shown focuses its first button. */
	void FocusUI();

	/** Routes input back to gameplay. */
	void FocusGame();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TMap<ESokobanDirection, TObjectPtr<UInputAction>> MoveActions;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UndoAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PaintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> EraseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RedoAction;

	/** One per brush, in ESokobanBrush order (keys 1-6). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> BrushActions;

	UPROPERTY(Transient)
	TObjectPtr<USokobanHUDWidget> HUD;

	UPROPERTY(Transient)
	TObjectPtr<USokobanMenuPanel> MenuPanel;

	UPROPERTY(Transient)
	TObjectPtr<USokobanLevelSelectWidget> LevelSelect;

	UPROPERTY(Transient)
	TObjectPtr<USokobanEditorWidget> EditorScreen;

	UPROPERTY(Transient)
	TObjectPtr<USokobanShelfWidget> ShelfScreen;

	/** Directions currently held, most recent last; the most recent one repeats. */
	TArray<ESokobanDirection> HeldDirections;

	/** When the most recent direction was pressed (world seconds). */
	double LastPressTime = 0.0;

	/** A direction pressed while the board was busy; it plays as soon as the board is free. */
	TOptional<ESokobanDirection> BufferedMove;

	/** World time when the board's current animation ends. */
	double BoardBusyUntil = 0.0;

	/** When a held key carried the hero on into the current walking step (negative when it didn't), and which way. */
	double CarriedStepTime = -1.0;
	ESokobanDirection CarriedStepDirection = ESokobanDirection::North;

	bool bUndoHeld = false;
	float UndoRepeatTimer = 0.f;
	float CurrentUndoInterval = 0.f;

	/** A brush stroke is under way, painting with the left button or erasing with the right. */
	bool bStroking = false;
	bool bStrokeErases = false;

	/** Moves still to play back from SokobanSolve. */
	TArray<ESokobanDirection> AutoMoves;
};
