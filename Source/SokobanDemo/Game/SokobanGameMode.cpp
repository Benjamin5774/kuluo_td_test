// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanGameMode.h"
#include "SokobanDemo.h"
#include "Game/SokobanPlayerController.h"
#include "Game/SokobanProgressSubsystem.h"
#include "View/SokobanBoardView.h"
#include "View/SokobanCameraRig.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
	/** Used when the level pack is missing or broken, so the game always has something to play. */
	FSokobanLevel MakeFallbackLevel()
	{
		FSokobanLevelData Data;
		Data.Id = TEXT("fallback");
		Data.Name = TEXT("Fallback");
		Data.Map = { TEXT("#######"), TEXT("#     #"), TEXT("#@ $ .#"), TEXT("#     #"), TEXT("#######") };

		FSokobanLevel Level;
		FString Error;
		FSokobanLevel::FromData(Data, Level, Error);
		return Level;
	}
}

ASokobanGameMode::ASokobanGameMode()
{
	PlayerControllerClass = ASokobanPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	BoardViewClass = ASokobanBoardView::StaticClass();
	CameraRigClass = ASokobanCameraRig::StaticClass();
}

void ASokobanGameMode::StartPlay()
{
	SpawnEnvironmentIfMissing();
	LoadLevels();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Use the board placed in the map (it previews a level in the editor), or spawn one at the origin.
	TActorIterator<ASokobanBoardView> PlacedBoard(GetWorld());
	BoardView = PlacedBoard ? *PlacedBoard : GetWorld()->SpawnActor<ASokobanBoardView>(BoardViewClass, FTransform::Identity, SpawnParams);
	CameraRig = GetWorld()->SpawnActor<ASokobanCameraRig>(CameraRigClass, FTransform::Identity, SpawnParams);

	// Zoomed out, the camera frames an area that holds the largest level, so the scenery placed around the
	// castle looks the same whichever level is on the board.
	FIntPoint LargestSize(0, 0);
	for (const FSokobanLevel& Level : Levels)
	{
		LargestSize = FIntPoint(FMath::Max(LargestSize.X, Level.Layout.Size.X), FMath::Max(LargestSize.Y, Level.Layout.Size.Y));
	}
	SceneBounds = BoardView->GetBoundsForSize(LargestSize);

	Super::StartPlay();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			PlayerController->SetViewTarget(CameraRig);
		}
	}

	// -SokobanLevel=N skips the menus and starts a level directly (handy for testing).
	int32 StartIndex = INDEX_NONE;
	if (FParse::Value(FCommandLine::Get(), TEXT("SokobanLevel="), StartIndex))
	{
		StartLevel(StartIndex);
	}
	else
	{
		ShowMainMenu();
	}
}

void ASokobanGameMode::LoadLevels()
{
	FString Error;
	const FString PackPath = SokobanLevelFiles::GetLevelsDir() / LevelPackFile;
	if (!SokobanLevelFiles::LoadPackLevels(PackPath, Levels, Error, &PackName))
	{
		UE_LOG(LogSokoban, Error, TEXT("Could not load level pack %s: %s"), *PackPath, *Error);
		Levels = { MakeFallbackLevel() };
	}
	UE_LOG(LogSokoban, Log, TEXT("Loaded %d levels from %s"), Levels.Num(), *PackPath);
}

void ASokobanGameMode::ShowMainMenu()
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);

	// The menu keeps a level on the board as its backdrop; after a win, show where the player will continue.
	if (LevelIndex == INDEX_NONE || bSolved)
	{
		LoadLevel(GetContinueIndex());
	}
	SetFlowState(ESokobanFlowState::MainMenu);
}

void ASokobanGameMode::ShowLevelSelect()
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	SetFlowState(ESokobanFlowState::LevelSelect);
}

void ASokobanGameMode::StartLevel(int32 Index)
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	LoadLevel(Index);
	if (USokobanProgressSubsystem* Progress = GetProgress())
	{
		Progress->SetLastLevelId(Levels[LevelIndex].Id);
	}
	SetFlowState(ESokobanFlowState::Playing);
}

void ASokobanGameMode::ContinueGame()
{
	StartLevel(GetContinueIndex());
}

void ASokobanGameMode::StartNextLevel()
{
	if (LevelIndex + 1 < Levels.Num())
	{
		StartLevel(LevelIndex + 1);
	}
	else
	{
		ShowLevelSelect();
	}
}

void ASokobanGameMode::PauseGame()
{
	if (FlowState == ESokobanFlowState::Playing)
	{
		SetFlowState(ESokobanFlowState::Paused);
	}
}

void ASokobanGameMode::ResumeGame()
{
	if (FlowState == ESokobanFlowState::Paused)
	{
		SetFlowState(ESokobanFlowState::Playing);
	}
}

void ASokobanGameMode::LoadLevel(int32 Index)
{
	LevelIndex = FMath::Clamp(Index, 0, Levels.Num() - 1);
	const FSokobanLevel& Level = Levels[LevelIndex];
	Puzzle.Load(Level);
	bSolved = false;
	LastAnimationTime = 0.f;

	BoardView->BuildLevel(Level);
	CameraRig->FrameBounds(BoardView->GetPlayBounds(), SceneBounds, false);

	UE_LOG(LogSokoban, Log, TEXT("Level %d/%d: %s (%s), par %d"), LevelIndex + 1, Levels.Num(), *Level.Name, *Level.Id, Level.Par);
	OnPuzzleChanged.Broadcast();
}

void ASokobanGameMode::SetFlowState(ESokobanFlowState NewState)
{
	FlowState = NewState;
	OnFlowStateChanged.Broadcast(NewState);
}

FSokobanMoveResult ASokobanGameMode::RequestMove(ESokobanDirection Direction)
{
	if (FlowState != ESokobanFlowState::Playing || bSolved)
	{
		LastAnimationTime = 0.f;
		return FSokobanMoveResult();
	}

	const FSokobanMoveResult Result = Puzzle.Move(Direction);
	LastAnimationTime = BoardView->PlayMove(Result, Puzzle.GetState());
	if (Result.Outcome == ESokobanMoveOutcome::Pushed)
	{
		// A thud only when the push sets crates down on goals; the more at once, the harder.
		int32 Landed = 0;
		for (int32 CrateId : Result.Row)
		{
			Landed += Puzzle.GetLevel().Layout.IsGoal(Puzzle.GetState().Crates[CrateId]) ? 1 : 0;
		}
		if (Landed > 0)
		{
			CameraRig->Jolt(FMath::Min(0.7f + 0.3f * (Landed - 1), 1.3f), LastAnimationTime);
		}
	}
	if (Result.DidMove())
	{
		OnPuzzleChanged.Broadcast();
		if (Puzzle.IsSolved())
		{
			HandleLevelSolved();
		}
	}
	return Result;
}

bool ASokobanGameMode::RetractLastWalk()
{
	if (FlowState != ESokobanFlowState::Playing || bSolved || !Puzzle.Undo())
	{
		return false;
	}

	BoardView->StopHero(Puzzle.GetActiveHero());
	LastAnimationTime = 0.f;
	OnPuzzleChanged.Broadcast();
	return true;
}

bool ASokobanGameMode::RequestUndo()
{
	if (FlowState != ESokobanFlowState::Playing || bSolved || !Puzzle.Undo())
	{
		return false;
	}

	BoardView->SyncToState(Puzzle.GetState(), UndoGlideTime);
	LastAnimationTime = UndoGlideTime;
	OnPuzzleChanged.Broadcast();
	return true;
}

bool ASokobanGameMode::RequestRestart()
{
	if (FlowState != ESokobanFlowState::Playing || bSolved || !Puzzle.Restart())
	{
		return false;
	}

	BoardView->SyncToState(Puzzle.GetState(), RestartGlideTime);
	LastAnimationTime = RestartGlideTime;
	OnPuzzleChanged.Broadcast();
	return true;
}

void ASokobanGameMode::HandleLevelSolved()
{
	bSolved = true;

	const FSokobanLevel& Level = Puzzle.GetLevel();
	LastCompletion.Moves = Puzzle.GetState().MoveCount;
	LastCompletion.Par = Level.Par;
	LastCompletion.Stars = FSokobanRules::StarsForSolve(LastCompletion.Moves, Level.Par);
	LastCompletion.bLastLevel = LevelIndex == Levels.Num() - 1;
	LastCompletion.bNewBest = false;
	if (USokobanProgressSubsystem* Progress = GetProgress())
	{
		LastCompletion.bNewBest = Progress->RecordSolve(Level.Id, LastCompletion.Moves);
	}
	UE_LOG(LogSokoban, Log, TEXT("Solved %s in %d moves (par %d)%s"), *Level.Id, LastCompletion.Moves, Level.Par, LastCompletion.bNewBest ? TEXT(", new best") : TEXT(""));

	// Celebrate once the final push has landed, then show the results. Both steps share ResultsTimer, so
	// leaving the level in between cancels whichever is pending.
	GetWorldTimerManager().SetTimer(ResultsTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		BoardView->PlayVictory();
		GetWorldTimerManager().SetTimer(ResultsTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetFlowState(ESokobanFlowState::LevelComplete);
		}), ResultsDelay, false);
	}), FMath::Max(LastAnimationTime, 0.01f), false);
}

bool ASokobanGameMode::IsLevelUnlocked(int32 Index) const
{
	if (!Levels.IsValidIndex(Index))
	{
		return false;
	}
	// Levels open one after another; a level stays open once solved.
	return Index == 0 || GetBestMoves(Index - 1) > 0 || GetBestMoves(Index) > 0;
}

int32 ASokobanGameMode::GetBestMoves(int32 Index) const
{
	const USokobanProgressSubsystem* Progress = GetProgress();
	return Progress && Levels.IsValidIndex(Index) ? Progress->GetBestMoves(Levels[Index].Id) : 0;
}

int32 ASokobanGameMode::GetContinueIndex() const
{
	for (int32 Index = 0; Index < Levels.Num(); ++Index)
	{
		if (GetBestMoves(Index) == 0)
		{
			return Index;
		}
	}

	// Everything is solved: resume the last level played.
	if (const USokobanProgressSubsystem* Progress = GetProgress())
	{
		const int32 Last = Levels.IndexOfByPredicate([Progress](const FSokobanLevel& Level) { return Level.Id == Progress->GetLastLevelId(); });
		if (Last != INDEX_NONE)
		{
			return Last;
		}
	}
	return 0;
}

bool ASokobanGameMode::HasAnyProgress() const
{
	for (int32 Index = 0; Index < Levels.Num(); ++Index)
	{
		if (GetBestMoves(Index) > 0)
		{
			return true;
		}
	}
	return false;
}

USokobanProgressSubsystem* ASokobanGameMode::GetProgress() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<USokobanProgressSubsystem>() : nullptr;
}

void ASokobanGameMode::SpawnEnvironmentIfMissing()
{
	UWorld* World = GetWorld();
	if (TActorIterator<ADirectionalLight>(World))
	{
		// The map brings its own lighting.
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Low warm sun from behind the camera's left shoulder, so shadows fall away from the player.
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1000.f), FRotator(-48.f, 35.f, 0.f), SpawnParams))
	{
		UDirectionalLightComponent* Light = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(6.f);
		Light->SetLightColor(FLinearColor(1.f, 0.9f, 0.78f));
	}

	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector(0.f, 0.f, 800.f), FRotator::ZeroRotator, SpawnParams))
	{
		USkyLightComponent* SkyLight = Sky->GetLightComponent();
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->RecaptureSky();
	}

	World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0.f, 0.f, -400.f), FRotator::ZeroRotator, SpawnParams))
	{
		Fog->GetComponent()->SetFogDensity(0.02f);
	}
}
