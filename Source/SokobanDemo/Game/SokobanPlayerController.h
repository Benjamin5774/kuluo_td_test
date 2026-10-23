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

/**
 * Turns input into puzzle moves and drives the UI.
 * Moves: one cell per press, steady repeats while held, hold-to-rewind with accelerating undo,
 * and a new press always cuts the running animation short. Screens follow the game mode's flow state.
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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanHUDWidget> HUDClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanMenuPanel> MenuPanelClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban|UI")
	TSubclassOf<USokobanLevelSelectWidget> LevelSelectClass;

	/** Seconds a direction must be held before it starts repeating. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Input")
	float MoveRepeatDelay = 0.2f;

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

	/** Sends a move to the game mode and returns how long its animation lasts. */
	float IssueMove(ESokobanDirection Direction);
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
	TObjectPtr<USokobanHUDWidget> HUD;

	UPROPERTY(Transient)
	TObjectPtr<USokobanMenuPanel> MenuPanel;

	UPROPERTY(Transient)
	TObjectPtr<USokobanLevelSelectWidget> LevelSelect;

	/** Directions currently held, most recent last; the most recent one repeats. */
	TArray<ESokobanDirection> HeldDirections;
	float MoveRepeatTimer = 0.f;

	bool bUndoHeld = false;
	float UndoRepeatTimer = 0.f;
	float CurrentUndoInterval = 0.f;

	/** Moves still to play back from SokobanSolve. */
	TArray<ESokobanDirection> AutoMoves;
	float AutoMoveTimer = 0.f;
};
