// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Puzzle/SokobanPuzzle.h"
#include "SokobanGameMode.generated.h"

class ASokobanBoardView;
class ASokobanCameraRig;
class USokobanProgressSubsystem;

/** Which screen the game is on. */
UENUM(BlueprintType)
enum class ESokobanFlowState : uint8
{
	MainMenu,
	LevelSelect,
	Playing,
	Paused,
	LevelComplete
};

/** Outcome of the level that was just solved, for the results screen. */
struct FSokobanCompletion
{
	int32 Moves = 0;
	int32 Par = 0;
	bool bNewBest = false;
	bool bLastLevel = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSokobanFlowStateChanged, ESokobanFlowState /*NewState*/);
DECLARE_MULTICAST_DELEGATE(FSokobanPuzzleChanged);

/**
 * Runs the game: level pack, screen flow and progress. Player requests go through here to the puzzle
 * (rules) and the board view (presentation); the UI listens to the two delegates below.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASokobanGameMode();

	virtual void StartPlay() override;

	/** No pawn: the player drives the puzzle directly and watches through the camera rig. */
	virtual void RestartPlayer(AController* NewPlayer) override {}

	void ShowMainMenu();
	void ShowLevelSelect();

	/** Loads a level and hands control to the player. */
	void StartLevel(int32 Index);

	/** Starts the first unsolved level, or the last one played once everything is solved. */
	void ContinueGame();

	/** Starts the level after the current one, or returns to level select after the last. */
	void StartNextLevel();

	void PauseGame();
	void ResumeGame();

	/** Moves the active hero; ignored unless playing. */
	FSokobanMoveResult RequestMove(ESokobanDirection Direction);
	bool RequestUndo();
	bool RequestRestart();

	ESokobanFlowState GetFlowState() const { return FlowState; }
	const FSokobanPuzzle& GetPuzzle() const { return Puzzle; }
	const TArray<FSokobanLevel>& GetLevels() const { return Levels; }
	const FString& GetPackName() const { return PackName; }

	/** Index of the level on the board, or INDEX_NONE before the first load. */
	int32 GetLevelIndex() const { return LevelIndex; }

	bool IsLevelUnlocked(int32 Index) const;

	/** Fewest moves the player has solved a level in, or 0 when unsolved. */
	int32 GetBestMoves(int32 Index) const;

	/** The level ContinueGame would start. */
	int32 GetContinueIndex() const;

	bool HasAnyProgress() const;

	const FSokobanCompletion& GetLastCompletion() const { return LastCompletion; }

	/** Length in seconds of the animation started by the last request. */
	float GetLastAnimationTime() const { return LastAnimationTime; }

	ASokobanCameraRig* GetCameraRig() const { return CameraRig; }
	ASokobanBoardView* GetBoardView() const { return BoardView; }

	/** Broadcast whenever the screen changes. */
	FSokobanFlowStateChanged OnFlowStateChanged;

	/** Broadcast whenever the board changes: level loaded, move, undo or restart. */
	FSokobanPuzzleChanged OnPuzzleChanged;

protected:
	/** Level pack to play, relative to Content/Levels. */
	UPROPERTY(EditAnywhere, Category = "Sokoban")
	FString LevelPackFile = TEXT("LevelPack.json");

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	TSubclassOf<ASokobanBoardView> BoardViewClass;

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	TSubclassOf<ASokobanCameraRig> CameraRigClass;

	/** Seconds of victory animation before the results screen appears. */
	UPROPERTY(EditAnywhere, Category = "Sokoban")
	float ResultsDelay = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	float UndoGlideTime = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Sokoban")
	float RestartGlideTime = 0.25f;

private:
	void LoadLevels();

	/** Shows a level on the board without changing the screen (also used as the menu backdrop). */
	void LoadLevel(int32 Index);

	void SetFlowState(ESokobanFlowState NewState);
	void HandleLevelSolved();
	void SpawnEnvironmentIfMissing();
	USokobanProgressSubsystem* GetProgress() const;

	FSokobanPuzzle Puzzle;
	TArray<FSokobanLevel> Levels;
	FString PackName;
	int32 LevelIndex = INDEX_NONE;
	ESokobanFlowState FlowState = ESokobanFlowState::MainMenu;

	/** The current level is solved; input stays locked until the next screen. */
	bool bSolved = false;

	FSokobanCompletion LastCompletion;
	float LastAnimationTime = 0.f;
	FTimerHandle ResultsTimer;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanBoardView> BoardView;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanCameraRig> CameraRig;
};
