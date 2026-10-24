// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanGameMode.h"
#include "SokobanDemo.h"
#include "Game/SokobanAudioSubsystem.h"
#include "Game/SokobanPlayerController.h"
#include "Game/SokobanProgressSubsystem.h"
#include "LevelEditor/SokobanLevelEditor.h"
#include "Puzzle/SokobanEditCanvas.h"
#include "View/SokobanBoardView.h"
#include "View/SokobanCameraRig.h"
#include "View/SokobanTitleStage.h"
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

	LevelEditor = GetWorld()->SpawnActor<ASokobanLevelEditor>(ASokobanLevelEditor::StaticClass(), FTransform::Identity, SpawnParams);
	LevelEditor->Init(BoardView, CameraRig);
	TitleStage = GetWorld()->SpawnActor<ASokobanTitleStage>(ASokobanTitleStage::StaticClass(), BoardView->GetActorTransform(), SpawnParams);

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
	LeaveDrawing();
	// The title screen shows a castle of its own, seen from the beach in front of it. The level being played
	// stays in the puzzle, to be picked up again (see ContinueGame).
	BoardView->BuildLevel(MakeTitleCastle(), true);
	bBoardShowsCampaign = false;
	TitleStage->Show();
	SetFlowState(ESokobanFlowState::MainMenu);
}

FSokobanLevel ASokobanGameMode::MakeTitleCastle()
{
	return FSokobanEditCanvas::MakeWalledRoom(FIntPoint(10, 6)).ToLevel(TEXT("title"), TEXT("Title"), 0);
}

void ASokobanGameMode::ShowLevelSelect()
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	LeaveDrawing();
	ShowBackdrop();
	SetFlowState(ESokobanFlowState::LevelSelect);
}

void ASokobanGameMode::ShowBackdrop()
{
	// The menus keep a game level on the board as their backdrop: the one being played, or after a win (and
	// coming from the title or the editor) where the player will continue.
	if (bBoardShowsCampaign && LevelIndex != INDEX_NONE && !bSolved)
	{
		return;
	}
	if (HasUnfinishedLevel())
	{
		ShowUnfinishedLevel();
	}
	else
	{
		LoadLevel(GetContinueIndex());
	}
}

void ASokobanGameMode::ShowUnfinishedLevel()
{
	BoardView->BuildLevel(Levels[LevelIndex]);
	BoardView->SyncToState(Puzzle.GetState(), 0.f);
	CameraRig->FrameBounds(BoardView->GetPlayBounds(), SceneBounds, false);
	bBoardShowsCampaign = true;
}

void ASokobanGameMode::StartLevel(int32 Index)
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	LeaveDrawing();
	LoadLevel(Index);
	if (USokobanProgressSubsystem* Progress = GetProgress())
	{
		Progress->SetLastLevelId(Levels[LevelIndex].Id);
	}
	SetFlowState(ESokobanFlowState::Playing);
}

bool ASokobanGameMode::HasUnfinishedLevel() const
{
	// Test plays and freshly loaded levels replace the puzzle, so only a game level left mid-play still holds moves.
	return PlayContext == ESokobanPlayContext::Campaign && LevelIndex != INDEX_NONE && !bSolved && Puzzle.GetState().MoveCount > 0;
}

void ASokobanGameMode::ContinueGame()
{
	if (HasUnfinishedLevel())
	{
		// Carry on from where the player left the level.
		GetWorldTimerManager().ClearTimer(ResultsTimer);
		LeaveDrawing();
		if (!bBoardShowsCampaign)
		{
			ShowUnfinishedLevel();
		}
		SetFlowState(ESokobanFlowState::Playing);
		return;
	}
	StartLevel(GetContinueIndex());
}

bool ASokobanGameMode::CanContinueGame() const
{
	return HasAnyProgress() || HasUnfinishedLevel();
}

void ASokobanGameMode::StartNewGame()
{
	if (USokobanProgressSubsystem* Progress = GetProgress())
	{
		Progress->ResetProgress();
	}
	StartLevel(0);
}

void ASokobanGameMode::LeaveDrawing()
{
	if (LevelEditor && FlowState == ESokobanFlowState::Editing)
	{
		LevelEditor->SaveDraftIfChanged();
		LevelEditor->Hide();
	}
}

void ASokobanGameMode::ShowEditorHome()
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	LeaveDrawing();
	ShowBackdrop();
	SetFlowState(ESokobanFlowState::EditorHome);
}

void ASokobanGameMode::ShowShelf(ESokobanShelf Shelf)
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	LeaveDrawing();
	ShowBackdrop();
	ShownShelf = Shelf;
	SetFlowState(ESokobanFlowState::EditorShelf);
}

FString ASokobanGameMode::MakeNewLevelName() const
{
	// The first number no level on either shelf is called by, so deleting one doesn't lead to two of a name.
	TSet<FString> Taken;
	for (const ESokobanShelf Shelf : { ESokobanShelf::Saved, ESokobanShelf::Drafts })
	{
		for (const FSokobanShelfEntry& Entry : SokobanShelf::List(SokobanShelf::GetDir(Shelf)))
		{
			Taken.Add(Entry.Data.Name);
		}
	}
	int32 Number = 1;
	while (Taken.Contains(FString::Printf(TEXT("自制关卡 %d"), Number)))
	{
		++Number;
	}
	return FString::Printf(TEXT("自制关卡 %d"), Number);
}

void ASokobanGameMode::DesignNewLevel()
{
	bBoardShowsCampaign = false;
	LevelEditor->Open(FSokobanEditCanvas::MakeWalledRoom(FIntPoint(8, 6)), SokobanShelf::MakeId(), MakeNewLevelName(), false);
	SetFlowState(ESokobanFlowState::Editing);
}

bool ASokobanGameMode::CanContinueDesign() const
{
	return LevelEditor->HasSession() || SokobanShelf::List(SokobanShelf::GetDir(ESokobanShelf::Drafts)).Num() > 0;
}

void ASokobanGameMode::ContinueDesign()
{
	if (LevelEditor->HasSession())
	{
		bBoardShowsCampaign = false;
		SetFlowState(ESokobanFlowState::Editing);
		LevelEditor->Show();
		return;
	}

	const TArray<FSokobanShelfEntry> Drafts = SokobanShelf::List(SokobanShelf::GetDir(ESokobanShelf::Drafts));
	if (Drafts.Num() > 0)
	{
		EditShelfLevel(ESokobanShelf::Drafts, Drafts[0].Data);
	}
}

void ASokobanGameMode::DeleteShelfLevel(ESokobanShelf Shelf, const FString& Id)
{
	SokobanShelf::Remove(SokobanShelf::GetDir(Shelf), Id);
	if (LevelEditor->GetLevelId() == Id)
	{
		// Otherwise leaving or continuing the drawing would store it again.
		LevelEditor->Close();
	}
	UE_LOG(LogSokoban, Log, TEXT("Deleted %s from %s"), *Id, Shelf == ESokobanShelf::Saved ? TEXT("saved levels") : TEXT("drafts"));
}

void ASokobanGameMode::EditShelfLevel(ESokobanShelf Shelf, const FSokobanLevelData& Data)
{
	// Drafts are stored exactly as drawn; saved levels are proper level files.
	FSokobanEditCanvas Canvas = FSokobanEditCanvas::FromRows(Data.Map, Data.Mechanisms);
	if (Shelf == ESokobanShelf::Saved)
	{
		FSokobanLevel Level;
		FString Error;
		if (FSokobanLevel::FromData(Data, Level, Error))
		{
			Canvas = FSokobanEditCanvas::FromLevel(Level);
		}
	}
	bBoardShowsCampaign = false;
	LevelEditor->Open(Canvas, Data.Id, Data.Name, Shelf == ESokobanShelf::Saved);
	SetFlowState(ESokobanFlowState::Editing);
}

void ASokobanGameMode::TestPlayDrawing()
{
	if (FlowState != ESokobanFlowState::Editing || !LevelEditor->CanTestPlay())
	{
		return;
	}
	// The player may leave for the menus from the test play: keep the drawing safe.
	LevelEditor->SaveDraftIfChanged();
	LevelEditor->Hide();
	StartTestPlay(LevelEditor->MakeTestLevel(), ESokobanPlayContext::EditorTest);
}

void ASokobanGameMode::TestPlayShelfLevel(const FSokobanLevelData& Data)
{
	FSokobanLevel Level;
	FString Error;
	if (FSokobanLevel::FromData(Data, Level, Error))
	{
		StartTestPlay(Level, ESokobanPlayContext::ShelfTest);
	}
	else
	{
		UE_LOG(LogSokoban, Warning, TEXT("Saved level %s doesn't parse: %s"), *Data.Id, *Error);
	}
}

void ASokobanGameMode::StartTestPlay(const FSokobanLevel& Level, ESokobanPlayContext Context)
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	PlayContext = Context;
	TestLevel = Level;
	Puzzle.Load(Level);
	bSolved = false;
	bBoardShowsCampaign = false;
	LastAnimationTime = 0.f;

	BoardView->BuildLevel(Level);
	CameraRig->FrameBounds(BoardView->GetPlayBounds(), SceneBounds, false);
	UE_LOG(LogSokoban, Log, TEXT("Test play: %s (%s), par %d"), *Level.Name, *Level.Id, Level.Par);
	OnPuzzleChanged.Broadcast();
	SetFlowState(ESokobanFlowState::Playing);
}

void ASokobanGameMode::ReplayTestLevel()
{
	StartTestPlay(TestLevel, PlayContext);
}

void ASokobanGameMode::LeaveTestPlay()
{
	GetWorldTimerManager().ClearTimer(ResultsTimer);
	if (PlayContext == ESokobanPlayContext::EditorTest)
	{
		SetFlowState(ESokobanFlowState::Editing);
		LevelEditor->Show();
	}
	else
	{
		ShowShelf(ESokobanShelf::Saved);
	}
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
	bBoardShowsCampaign = true;
	PlayContext = ESokobanPlayContext::Campaign;
	LastAnimationTime = 0.f;

	BoardView->BuildLevel(Level);
	CameraRig->FrameBounds(BoardView->GetPlayBounds(), SceneBounds, false);

	UE_LOG(LogSokoban, Log, TEXT("Level %d/%d: %s (%s), par %d"), LevelIndex + 1, Levels.Num(), *Level.Name, *Level.Id, Level.Par);
	OnPuzzleChanged.Broadcast();
}

void ASokobanGameMode::SetFlowState(ESokobanFlowState NewState)
{
	if (NewState != ESokobanFlowState::MainMenu && TitleStage && TitleStage->IsShown())
	{
		TitleStage->Hide();
	}
	if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
	{
		Audio->SetMusic(NewState == ESokobanFlowState::MainMenu ? ESokobanMusic::Title : ESokobanMusic::Game);
		// The results bring the music back once their stars have landed; anywhere else it comes back at once.
		if (NewState != ESokobanFlowState::LevelComplete)
		{
			Audio->ResumeMusic();
		}
	}
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
	USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this);
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
		if (Audio)
		{
			// The scrape grows heavier with the length of the row; a chime as crates settle on goals.
			Audio->Play(ESokobanSound::Push, FMath::Min(0.6f + 0.1f * Result.Row.Num(), 1.f), 1.f - 0.04f * (Result.Row.Num() - 1));
			if (Landed > 0)
			{
				Audio->Play(ESokobanSound::Goal, 0.85f, 1.f + 0.06f * (Landed - 1), LastAnimationTime);
			}
		}
	}
	else if (Audio && Result.Outcome == ESokobanMoveOutcome::Walked)
	{
		// Two soft footfalls per cell, the second a shade quieter.
		Audio->Play(ESokobanSound::Step, 0.5f, 1.f, LastAnimationTime * 0.3f);
		Audio->Play(ESokobanSound::Step, 0.38f, 0.93f, LastAnimationTime * 0.8f);
	}
	else if (Audio)
	{
		Audio->Play(ESokobanSound::Bump, 0.45f);
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
	if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
	{
		Audio->Play(ESokobanSound::Undo, 0.5f);
	}
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
	if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
	{
		Audio->Play(ESokobanSound::Undo, 0.7f, 0.75f);
	}
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
	LastCompletion.bLastLevel = PlayContext == ESokobanPlayContext::Campaign && LevelIndex == Levels.Num() - 1;
	LastCompletion.bNewBest = false;
	if (PlayContext == ESokobanPlayContext::Campaign)
	{
		if (USokobanProgressSubsystem* Progress = GetProgress())
		{
			LastCompletion.bNewBest = Progress->RecordSolve(Level.Id, LastCompletion.Moves);
		}
	}
	else if (PlayContext == ESokobanPlayContext::EditorTest)
	{
		// Clearing a test play proves the drawing solvable, even when the solver couldn't tell.
		LevelEditor->NoteTestPlaySolved(LastCompletion.Moves);
	}
	UE_LOG(LogSokoban, Log, TEXT("Solved %s in %d moves (par %d)%s"), *Level.Id, LastCompletion.Moves, Level.Par, LastCompletion.bNewBest ? TEXT(", new best") : TEXT(""));

	// Celebrate once the final push has landed, then show the results. Both steps share ResultsTimer, so
	// leaving the level in between cancels whichever is pending.
	GetWorldTimerManager().SetTimer(ResultsTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		BoardView->PlayVictory();
		if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
		{
			// The music makes way for the fanfare and the stars.
			Audio->Play(ESokobanSound::Win, 0.9f);
			Audio->HoldMusic();
		}
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
