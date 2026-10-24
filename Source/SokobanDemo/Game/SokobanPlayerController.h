// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Puzzle/SokobanTypes.h"
#include "SokobanPlayerController.generated.h"

class ASokobanGameMode;
class UInputAction;
class UInputMappingContext;
class USokobanHUDWidget;
class USokobanLevelSelectWidget;
class USokobanMenuPanel;
class UUserWidget;
enum class ESokobanFlowState : uint8;
struct FInputActionValue;

/**
 * Turns input into puzzle moves and drives the UI.
 * Moves: one cell per press and steady repeats while held. A move always plays out: a press during it is
 * kept and played the moment it lands, so steps join into one continuous walk. Undo rewinds at once, and
 * holding it rewinds faster and faster. Screens follow the game mode's flow state.
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

	/** Dev: opens a screen (menu, levels or pause) after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanScreen(const FString& Screen, float Delay = 0.f);

	/** Dev: forgets all saved progress. */
	UFUNCTION(Exec)
	void SokobanResetProgress();

	/** Dev: logs the board pieces after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanDump(float Delay = 0.f);

	/** Dev: zooms the camera by Steps wheel notches (positive in) after Delay seconds. */
	UFUNCTION(Exec)
	void SokobanZoom(float Steps, float Delay = 0.f);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanHUDWidget> HUDClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanMenuPanel> MenuPanelClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanLevelSelectWidget> LevelSelectClass;

	/** Seconds a direction must be held before it starts repeating (it repeats when the current move lands). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float MoveRepeatDelay = 0.12f;

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

	/** Sends a move to the game mode; the next one waits until its animation has played out. */
	void IssueMove(ESokobanDirection Direction);

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

	/** Routes keyboard / gamepad input to Widget and shows the cursor. */
	void FocusUI(UUserWidget* Widget);

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
	TObjectPtr<USokobanHUDWidget> HUD;

	UPROPERTY(Transient)
	TObjectPtr<USokobanMenuPanel> MenuPanel;

	UPROPERTY(Transient)
	TObjectPtr<USokobanLevelSelectWidget> LevelSelect;

	/** Directions currently held, most recent last; the most recent one repeats. */
	TArray<ESokobanDirection> HeldDirections;

	/** When the most recent direction was pressed (world seconds). */
	double LastPressTime = 0.0;

	/** A direction pressed while the board was busy; it plays as soon as the board is free. */
	TOptional<ESokobanDirection> BufferedMove;

	/** World time when the board's current animation ends. */
	double BoardBusyUntil = 0.0;

	bool bUndoHeld = false;
	float UndoRepeatTimer = 0.f;
	float CurrentUndoInterval = 0.f;

	/** Moves still to play back from SokobanSolve. */
	TArray<ESokobanDirection> AutoMoves;
};
