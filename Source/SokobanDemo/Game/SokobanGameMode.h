// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LevelEditor/SokobanLevelShelf.h"
#include "Puzzle/SokobanPuzzle.h"
#include "SokobanGameMode.generated.h"

class ASokobanBoardView;
class ASokobanCameraRig;
class ASokobanLevelEditor;
class ASokobanTitleStage;
class USokobanProgressSubsystem;

/** Which screen the game is on. */
UENUM(BlueprintType)
enum class ESokobanFlowState : uint8
{
	MainMenu,
	LevelSelect,
	Playing,
	Paused,
	LevelComplete,
	/** The level editor's start page: design, continue, browse. */
	EditorHome,
	/** A list of levels made in the editor (saved or drafts, see GetShelf). */
	EditorShelf,
	/** Drawing a level. */
	Editing
};

/** What is being played, which decides where the player returns afterwards. */
enum class ESokobanPlayContext : uint8
{
	/** The game's own levels, with progress. */
	Campaign,
	/** A test play of the drawing in the editor. */
	EditorTest,
	/** A test play of a saved level, from the editor's list. */
	ShelfTest
};

/** Outcome of the level that was just solved, for the results screen. */
struct FSokobanCompletion
{
	int32 Moves = 0;
	int32 Par = 0;
	/** Stars this solve earned, 1 to FSokobanRules::MaxStars. */
	int32 Stars = 0;
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

	/**
	 * Picks up the level the player left half-played for the menus, if the board still shows it; otherwise
	 * starts the first unsolved level, or the last one played once everything is solved.
	 */
	void ContinueGame();

	/** Whether there is anything to continue: progress, or a level left half-played. */
	bool CanContinueGame() const;

	/** Forgets all progress and starts from the first level. */
	void StartNewGame();

	/** The level editor's start page. Leaving the drawing on the way stores a draft of any changes. */
	void ShowEditorHome();

	/** The editor's list of saved levels or drafts. */
	void ShowShelf(ESokobanShelf Shelf);
	ESokobanShelf GetShelf() const { return ShownShelf; }

	/** Starts drawing a new level: an empty walled room. */
	void DesignNewLevel();

	/** Back to the drawing left this run, undo history and all; after a restart, the most recently changed draft. */
	void ContinueDesign();
	bool CanContinueDesign() const;

	/** Opens a level from a shelf in the editor. */
	void EditShelfLevel(ESokobanShelf Shelf, const FSokobanLevelData& Data);

	/** Deletes a level from a shelf. Deleting the level being designed discards the drawing too. */
	void DeleteShelfLevel(ESokobanShelf Shelf, const FString& Id);

	/** Test-plays the drawing, or a saved level; LeaveTestPlay returns to where it started. */
	void TestPlayDrawing();
	void TestPlayShelfLevel(const FSokobanLevelData& Data);
	void LeaveTestPlay();

	/** Plays the same test level again from the start. */
	void ReplayTestLevel();

	ESokobanPlayContext GetPlayContext() const { return PlayContext; }
	ASokobanLevelEditor* GetLevelEditor() const { return LevelEditor; }

	/** The title screen's scene, shown with the main menu. */
	ASokobanTitleStage* GetTitleStage() const { return TitleStage; }

	/** Starts the level after the current one, or returns to level select after the last. */
	void StartNextLevel();

	void PauseGame();
	void ResumeGame();

	/** Moves the active hero; ignored unless playing. */
	FSokobanMoveResult RequestMove(ESokobanDirection Direction);
	bool RequestUndo();
	bool RequestRestart();

	/**
	 * Takes back the last move, which the caller knows was a plain walk, leaving the hero where it is drawn
	 * mid-step so the next move sets off from there. Used to turn a corner the player reached a moment late.
	 */
	bool RetractLastWalk();

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

	/** Puts a game level back on the board for the menus, if the board shows something else. */
	void ShowBackdrop();

	/** The game level being played was left half-played (moves made, not solved) for the menus. */
	bool HasUnfinishedLevel() const;

	/** Puts that level back on the board as it was left. */
	void ShowUnfinishedLevel();

	/** The castle the title screen shows: a plain walled courtyard, built as a showcase. */
	static FSokobanLevel MakeTitleCastle();

	/** Leaves the drawing: stores a draft of any changes and takes it off the board. */
	void LeaveDrawing();

	void StartTestPlay(const FSokobanLevel& Level, ESokobanPlayContext Context);

	/** A name for a new level: "自制关卡 N". */
	FString MakeNewLevelName() const;

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

	/** The area the camera frames when zoomed out: room for the largest level of the pack. */
	FBox SceneBounds = FBox(ForceInit);

	ESokobanPlayContext PlayContext = ESokobanPlayContext::Campaign;

	/** The level being test-played, for playing it again. */
	FSokobanLevel TestLevel;

	/** Whether the board shows the game level at LevelIndex (rather than a drawing or a test level). */
	bool bBoardShowsCampaign = false;

	ESokobanShelf ShownShelf = ESokobanShelf::Saved;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanBoardView> BoardView;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanCameraRig> CameraRig;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanLevelEditor> LevelEditor;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanTitleStage> TitleStage;
};
