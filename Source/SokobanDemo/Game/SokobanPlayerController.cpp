// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanPlayerController.h"
#include "SokobanDemo.h"
#include "Game/SokobanGameMode.h"
#include "Game/SokobanProgressSubsystem.h"
#include "LevelEditor/SokobanLevelEditor.h"
#include "LevelEditor/SokobanLevelShelf.h"
#include "Puzzle/SokobanEditCanvas.h"
#include "Puzzle/SokobanSolver.h"
#include "UI/SokobanEditorWidget.h"
#include "UI/SokobanHUDWidget.h"
#include "UI/SokobanLevelSelectWidget.h"
#include "UI/SokobanMenuPanel.h"
#include "UI/SokobanShelfWidget.h"
#include "View/SokobanBoardView.h"
#include "View/SokobanCameraRig.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "SokobanPlayerController"

namespace
{
	/** Where leaving a test play returns to. */
	FText DescribeTestReturn(ESokobanPlayContext Context)
	{
		return Context == ESokobanPlayContext::EditorTest ? LOCTEXT("BackToEditor", "返回编辑器") : LOCTEXT("BackToSaved", "返回保存的关卡");
	}

	/** A draft's state in a few words, for the drafts list. */
	FText DescribeDraft(const FSokobanEditCanvas& Canvas)
	{
		const TArray<ESokobanCanvasIssue> Issues = Canvas.FindIssues();
		if (Issues.Num() == 0)
		{
			return LOCTEXT("DraftReady", "可以试玩");
		}
		switch (Issues[0])
		{
		case ESokobanCanvasIssue::NoHero:
			return LOCTEXT("DraftNoHero", "还没放大橘");
		case ESokobanCanvasIssue::NoGoal:
			return LOCTEXT("DraftNoGoal", "还没有目标点");
		case ESokobanCanvasIssue::TooFewCrates:
			return LOCTEXT("DraftFewCrates", "箱子不够");
		default:
			return LOCTEXT("DraftOpen", "墙没围好");
		}
	}

	/** Dev commands: a brush by name (floor, wall, crate, goal, hero or erase). */
	bool ParseBrush(const FString& Name, ESokobanBrush& OutBrush)
	{
		static const TMap<FString, ESokobanBrush> Names = {
			{ TEXT("floor"), ESokobanBrush::Floor }, { TEXT("wall"), ESokobanBrush::Wall }, { TEXT("crate"), ESokobanBrush::Crate },
			{ TEXT("goal"), ESokobanBrush::Goal }, { TEXT("hero"), ESokobanBrush::Hero }, { TEXT("erase"), ESokobanBrush::Erase } };
		if (const ESokobanBrush* Found = Names.Find(Name.ToLower()))
		{
			OutBrush = *Found;
			return true;
		}
		UE_LOG(LogSokoban, Warning, TEXT("Unknown brush '%s' (floor, wall, crate, goal, hero or erase)."), *Name);
		return false;
	}

	/** When a shelf file last changed, in local time. */
	FText DescribeModified(const FDateTime& Utc)
	{
		const FDateTime Local = Utc + (FDateTime::Now() - FDateTime::UtcNow());
		return FText::FromString(FString::Printf(TEXT("%d月%d日 %02d:%02d"), Local.GetMonth(), Local.GetDay(), Local.GetHour(), Local.GetMinute()));
	}
}

ASokobanPlayerController::ASokobanPlayerController()
{
	// The camera rig is the view target for the whole session; never swap it for a pawn.
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = false;

	HUDClass = USokobanHUDWidget::StaticClass();
	MenuPanelClass = USokobanMenuPanel::StaticClass();
	LevelSelectClass = USokobanLevelSelectWidget::StaticClass();
	EditorScreenClass = USokobanEditorWidget::StaticClass();
	ShelfScreenClass = USokobanShelfWidget::StaticClass();
}

void ASokobanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateInputMappings();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}

	if (IsLocalController())
	{
		CreateWidgets();
	}

	if (ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		if (ASokobanCameraRig* CameraRig = GameMode->GetCameraRig())
		{
			SetViewTarget(CameraRig);
		}
		GameMode->OnFlowStateChanged.AddUObject(this, &ASokobanPlayerController::HandleFlowStateChanged);
		GameMode->OnPuzzleChanged.AddUObject(this, &ASokobanPlayerController::HandlePuzzleChanged);
		if (EditorScreen)
		{
			EditorScreen->Bind(GameMode->GetLevelEditor());
		}
	}
}

void ASokobanPlayerController::CreateWidgets()
{
	HUD = CreateWidget<USokobanHUDWidget>(this, HUDClass);
	HUD->OnPause = [this]() { OnPausePressed(); };
	HUD->AddToViewport(0);
	HUD->SetVisibility(ESlateVisibility::Collapsed);

	EditorScreen = CreateWidget<USokobanEditorWidget>(this, EditorScreenClass);
	EditorScreen->OnTestPlay = [this]() { if (ASokobanGameMode* GameMode = GetSokobanGameMode()) { GameMode->TestPlayDrawing(); } };
	EditorScreen->OnBack = [this]() { if (ASokobanGameMode* GameMode = GetSokobanGameMode()) { GameMode->ShowEditorHome(); } };
	EditorScreen->AddToViewport(5);
	EditorScreen->SetVisibility(ESlateVisibility::Collapsed);

	MenuPanel = CreateWidget<USokobanMenuPanel>(this, MenuPanelClass);
	MenuPanel->AddToViewport(10);
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);

	LevelSelect = CreateWidget<USokobanLevelSelectWidget>(this, LevelSelectClass);
	LevelSelect->AddToViewport(10);
	LevelSelect->SetVisibility(ESlateVisibility::Collapsed);

	ShelfScreen = CreateWidget<USokobanShelfWidget>(this, ShelfScreenClass);
	ShelfScreen->AddToViewport(10);
	ShelfScreen->SetVisibility(ESlateVisibility::Collapsed);
}

void ASokobanPlayerController::CreateInputMappings()
{
	if (MappingContext)
	{
		return;
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Sokoban"));
	auto MakeAction = [this](const TCHAR* Name, std::initializer_list<FKey> Keys)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		for (const FKey& Key : Keys)
		{
			MappingContext->MapKey(Action, Key);
		}
		return Action;
	};

	MoveActions.Add(ESokobanDirection::North, MakeAction(TEXT("IA_MoveNorth"), { EKeys::W, EKeys::Up, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up }));
	MoveActions.Add(ESokobanDirection::South, MakeAction(TEXT("IA_MoveSouth"), { EKeys::S, EKeys::Down, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down }));
	MoveActions.Add(ESokobanDirection::West, MakeAction(TEXT("IA_MoveWest"), { EKeys::A, EKeys::Left, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left }));
	MoveActions.Add(ESokobanDirection::East, MakeAction(TEXT("IA_MoveEast"), { EKeys::D, EKeys::Right, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right }));
	UndoAction = MakeAction(TEXT("IA_Undo"), { EKeys::Z, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right });
	RestartAction = MakeAction(TEXT("IA_Restart"), { EKeys::R, EKeys::Gamepad_FaceButton_Top });
	PauseAction = MakeAction(TEXT("IA_Pause"), { EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right });

	// The level editor: the mouse paints and erases, the number keys pick a brush, and Z (above) undoes.
	PaintAction = MakeAction(TEXT("IA_Paint"), { EKeys::LeftMouseButton });
	EraseAction = MakeAction(TEXT("IA_Erase"), { EKeys::RightMouseButton });
	RedoAction = MakeAction(TEXT("IA_Redo"), { EKeys::Y });
	const FKey BrushKeys[][2] = {
		{ EKeys::One, EKeys::NumPadOne }, { EKeys::Two, EKeys::NumPadTwo }, { EKeys::Three, EKeys::NumPadThree },
		{ EKeys::Four, EKeys::NumPadFour }, { EKeys::Five, EKeys::NumPadFive }, { EKeys::Six, EKeys::NumPadSix } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BrushKeys); ++Index)
	{
		BrushActions.Add(MakeAction(*FString::Printf(TEXT("IA_Brush%d"), Index + 1), { BrushKeys[Index][0], BrushKeys[Index][1] }));
	}

	// The wheel reports one unit per notch, positive when rolled away (zoom in).
	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;
	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
}

void ASokobanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	CreateInputMappings();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogSokoban, Error, TEXT("SokobanPlayerController needs Enhanced Input as the default input component."));
		return;
	}

	for (const TPair<ESokobanDirection, TObjectPtr<UInputAction>>& Pair : MoveActions)
	{
		Input->BindAction(Pair.Value, ETriggerEvent::Started, this, &ASokobanPlayerController::OnDirectionPressed, Pair.Key);
		Input->BindAction(Pair.Value, ETriggerEvent::Completed, this, &ASokobanPlayerController::OnDirectionReleased, Pair.Key);
	}
	Input->BindAction(UndoAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnUndoPressed);
	Input->BindAction(UndoAction, ETriggerEvent::Completed, this, &ASokobanPlayerController::OnUndoReleased);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnRestartPressed);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnPausePressed);
	Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ASokobanPlayerController::OnZoom);
	Input->BindAction(PaintAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnStrokePressed, false);
	Input->BindAction(PaintAction, ETriggerEvent::Completed, this, &ASokobanPlayerController::OnStrokeReleased, false);
	Input->BindAction(EraseAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnStrokePressed, true);
	Input->BindAction(EraseAction, ETriggerEvent::Completed, this, &ASokobanPlayerController::OnStrokeReleased, true);
	Input->BindAction(RedoAction, ETriggerEvent::Started, this, &ASokobanPlayerController::OnRedoPressed);
	for (int32 Index = 0; Index < BrushActions.Num(); ++Index)
	{
		Input->BindAction(BrushActions[Index], ETriggerEvent::Started, this, &ASokobanPlayerController::OnBrushPressed, Index);
	}
}

void ASokobanPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Moves never cut each other short. The next one (a press kept from during the last move, a held key or
	// solver playback) starts the moment the board is free, so consecutive steps join into one walk.
	if (IsPlaying() && !IsBoardBusy())
	{
		if (BufferedMove.IsSet())
		{
			const ESokobanDirection Next = BufferedMove.GetValue();
			BufferedMove.Reset();
			IssueMove(Next);
		}
		else if (HeldDirections.Num() > 0 && GetWorld()->GetTimeSeconds() - LastPressTime >= MoveRepeatDelay)
		{
			// Note a walking step the held key carried the hero on into: a turn pressed a moment later
			// takes it back (TryTurnLate).
			const ESokobanDirection Next = HeldDirections.Last();
			const bool bWalked = IssueMove(Next).Outcome == ESokobanMoveOutcome::Walked;
			CarriedStepTime = bWalked ? GetWorld()->GetTimeSeconds() : -1.0;
			CarriedStepDirection = Next;
		}
		else if (AutoMoves.Num() > 0)
		{
			const ESokobanDirection Next = AutoMoves[0];
			AutoMoves.RemoveAt(0);
			IssueMove(Next);
		}
	}

	if (bUndoHeld)
	{
		UndoRepeatTimer -= DeltaTime;
		if (UndoRepeatTimer <= 0.f)
		{
			ASokobanGameMode* GameMode = GetSokobanGameMode();
			CarriedStepTime = -1.0;
			if (GameMode && GameMode->RequestUndo())
			{
				MarkBoardBusy();
			}
			CurrentUndoInterval = FMath::Max(UndoRepeatMinInterval, CurrentUndoInterval * UndoRepeatAcceleration);
			UndoRepeatTimer = CurrentUndoInterval;
		}
	}
}

void ASokobanPlayerController::OnDirectionPressed(ESokobanDirection Direction)
{
	if (!IsPlaying())
	{
		return;
	}

	AutoMoves.Reset();
	HeldDirections.Remove(Direction);
	HeldDirections.Add(Direction);
	LastPressTime = GetWorld()->GetTimeSeconds();

	if (!IsBoardBusy())
	{
		IssueMove(Direction);
	}
	else if (!TryTurnLate(Direction))
	{
		// Keep the latest press; it plays when the running move lands.
		BufferedMove = Direction;
	}
}

bool ASokobanPlayerController::TryTurnLate(ESokobanDirection Direction)
{
	// The held key carried the hero on into another step just before this turn came in: the player meant
	// to turn at the cell it was leaving, so take that step back and turn there instead.
	if (CarriedStepTime < 0.0 || Direction == CarriedStepDirection || GetWorld()->GetTimeSeconds() - CarriedStepTime > TurnGrace)
	{
		return false;
	}

	CarriedStepTime = -1.0;
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!GameMode || !GameMode->RetractLastWalk())
	{
		return false;
	}
	BufferedMove.Reset();
	IssueMove(Direction);
	return true;
}

void ASokobanPlayerController::OnDirectionReleased(ESokobanDirection Direction)
{
	HeldDirections.Remove(Direction);
}

void ASokobanPlayerController::OnUndoPressed()
{
	if (ASokobanLevelEditor* Editor = GetDrawingEditor())
	{
		if (!bStroking)
		{
			Editor->Undo();
		}
		return;
	}

	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!GameMode || !IsPlaying())
	{
		return;
	}

	AutoMoves.Reset();
	BufferedMove.Reset();
	CarriedStepTime = -1.0;
	if (GameMode->RequestUndo())
	{
		MarkBoardBusy();
	}
	bUndoHeld = true;
	UndoRepeatTimer = UndoRepeatDelay;
	CurrentUndoInterval = UndoRepeatInterval;
}

void ASokobanPlayerController::OnUndoReleased()
{
	bUndoHeld = false;
}

void ASokobanPlayerController::OnRestartPressed()
{
	if (ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		AutoMoves.Reset();
		BufferedMove.Reset();
		CarriedStepTime = -1.0;
		if (GameMode->RequestRestart())
		{
			MarkBoardBusy();
		}
	}
}

void ASokobanPlayerController::OnPausePressed()
{
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!GameMode)
	{
		return;
	}

	if (GameMode->GetFlowState() != ESokobanFlowState::Editing)
	{
		GameMode->PauseGame();
	}
	else if (GetDrawingEditor())
	{
		// Like the editor's back button.
		GameMode->ShowEditorHome();
	}
}

void ASokobanPlayerController::OnZoom(const FInputActionValue& Value)
{
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (GameMode && GameMode->GetCameraRig() && (IsPlaying() || GetDrawingEditor()))
	{
		GameMode->GetCameraRig()->AddZoom(Value.Get<float>());
	}
}

void ASokobanPlayerController::OnStrokePressed(bool bErase)
{
	// One stroke at a time: the other button does nothing until it ends.
	ASokobanLevelEditor* Editor = GetDrawingEditor();
	if (Editor && !bStroking)
	{
		bStroking = true;
		bStrokeErases = bErase;
		Editor->BeginStroke(bErase);
	}
}

void ASokobanPlayerController::OnStrokeReleased(bool bErase)
{
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (bStroking && bStrokeErases == bErase && GameMode)
	{
		bStroking = false;
		GameMode->GetLevelEditor()->EndStroke();
	}
}

void ASokobanPlayerController::OnRedoPressed()
{
	ASokobanLevelEditor* Editor = GetDrawingEditor();
	if (Editor && !bStroking)
	{
		Editor->Redo();
	}
}

void ASokobanPlayerController::OnBrushPressed(int32 Brush)
{
	if (ASokobanLevelEditor* Editor = GetDrawingEditor())
	{
		Editor->SetBrush(static_cast<ESokobanBrush>(Brush));
	}
}

ASokobanLevelEditor* ASokobanPlayerController::GetDrawingEditor() const
{
	const ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!GameMode || GameMode->GetFlowState() != ESokobanFlowState::Editing || (EditorScreen && EditorScreen->IsEditingName()))
	{
		return nullptr;
	}
	return GameMode->GetLevelEditor();
}

FSokobanMoveResult ASokobanPlayerController::IssueMove(ESokobanDirection Direction)
{
	// Any move other than a held key's repeat (which notes itself afterwards) can't be taken back for a late turn.
	CarriedStepTime = -1.0;
	FSokobanMoveResult Result;
	if (ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		Result = GameMode->RequestMove(Direction);
		MarkBoardBusy();
	}
	return Result;
}

bool ASokobanPlayerController::IsBoardBusy() const
{
	// Free from the frame the move lands in (with a hair of slack for rounding), never earlier: the board
	// starts the next move exactly at that landing, and starting it early would hold the hero still.
	return GetWorld()->GetTimeSeconds() + 0.001 < BoardBusyUntil;
}

void ASokobanPlayerController::MarkBoardBusy()
{
	if (const ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		BoardBusyUntil = GetWorld()->GetTimeSeconds() + GameMode->GetLastAnimationTime();
	}
}

bool ASokobanPlayerController::IsPlaying() const
{
	const ASokobanGameMode* GameMode = GetSokobanGameMode();
	return GameMode && GameMode->GetFlowState() == ESokobanFlowState::Playing;
}

ASokobanGameMode* ASokobanPlayerController::GetSokobanGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ASokobanGameMode>() : nullptr;
}

void ASokobanPlayerController::HandleFlowStateChanged(ESokobanFlowState State)
{
	HeldDirections.Reset();
	BufferedMove.Reset();
	BoardBusyUntil = 0.0;
	CarriedStepTime = -1.0;
	bUndoHeld = false;
	bStroking = false;
	if (State != ESokobanFlowState::Playing)
	{
		AutoMoves.Reset();
	}

	if (!HUD)
	{
		return;
	}

	// Of the HUD and the editor screen only the buttons take clicks; the rest of the screen is the board's.
	const bool bInLevel = State == ESokobanFlowState::Playing || State == ESokobanFlowState::Paused || State == ESokobanFlowState::LevelComplete;
	HUD->SetVisibility(bInLevel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	EditorScreen->SetVisibility(State == ESokobanFlowState::Editing ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	LevelSelect->SetVisibility(ESlateVisibility::Collapsed);
	ShelfScreen->SetVisibility(ESlateVisibility::Collapsed);

	switch (State)
	{
	case ESokobanFlowState::MainMenu:
		PresentMainMenu();
		break;
	case ESokobanFlowState::LevelSelect:
		PresentLevelSelect();
		break;
	case ESokobanFlowState::Paused:
		PresentPauseMenu();
		break;
	case ESokobanFlowState::LevelComplete:
		PresentResults();
		break;
	case ESokobanFlowState::EditorHome:
		PresentEditorHome();
		break;
	case ESokobanFlowState::EditorShelf:
		PresentShelf();
		break;
	case ESokobanFlowState::Editing:
		PresentDrawing();
		break;
	default:
		FocusGame();
		break;
	}
}

void ASokobanPlayerController::HandlePuzzleChanged()
{
	const ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!HUD || !GameMode)
	{
		return;
	}

	const FSokobanLevel& Level = GameMode->GetPuzzle().GetLevel();
	const bool bCampaign = GameMode->GetPlayContext() == ESokobanPlayContext::Campaign;
	HUD->SetLevelTitle(bCampaign
		? FText::Format(LOCTEXT("LevelTitle", "第 {0} 关 · {1}"), GameMode->GetLevelIndex() + 1, FText::FromString(Level.Name))
		: FText::Format(LOCTEXT("TestTitle", "试玩 · {0}"), FText::FromString(Level.Name)));

	TArray<FText> Stats;
	Stats.Add(FText::Format(LOCTEXT("Moves", "步数 {0}"), GameMode->GetPuzzle().GetState().MoveCount));
	if (Level.Par > 0)
	{
		Stats.Add(FText::Format(LOCTEXT("Par", "满星 {0}"), Level.Par));
	}
	if (const int32 Best = bCampaign ? GameMode->GetBestMoves(GameMode->GetLevelIndex()) : 0)
	{
		Stats.Add(FText::Format(LOCTEXT("Best", "最佳 {0}"), Best));
	}
	HUD->SetStats(FText::Join(FText::FromString(TEXT("      ")), Stats));
}

void ASokobanPlayerController::PresentMainMenu()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	TWeakObjectPtr<ASokobanPlayerController> WeakThis = this;
	if (!GameMode.IsValid())
	{
		return;
	}

	// Starting over wipes progress, which deserves a second thought.
	const bool bHasProgress = GameMode->HasAnyProgress();
	auto StartNewGame = [GameMode, WeakThis, bHasProgress]()
	{
		if (!GameMode.IsValid() || !WeakThis.IsValid())
		{
			return;
		}
		if (!bHasProgress)
		{
			GameMode->StartNewGame();
			return;
		}
		WeakThis->PresentConfirm(LOCTEXT("NewGameTitle", "开始新游戏？"),
			LOCTEXT("NewGameMessage", "通关记录和最佳步数会被清除，\n从第 1 关重新开始。\n自制的关卡不受影响。"),
			LOCTEXT("NewGameConfirm", "清除进度，重新开始"),
			[GameMode]() { if (GameMode.IsValid()) { GameMode->StartNewGame(); } },
			[WeakThis]() { if (WeakThis.IsValid()) { WeakThis->PresentMainMenu(); } });
	};

	TArray<FSokobanMenuItem> Items;
	Items.Add({ LOCTEXT("Continue", "继续"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ContinueGame(); } }, GameMode->CanContinueGame() });
	Items.Add({ LOCTEXT("NewGame", "开始新游戏"), StartNewGame });
	Items.Add({ LOCTEXT("SelectLevel", "选择关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } } });
	Items.Add({ LOCTEXT("Editor", "关卡编辑器"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowEditorHome(); } } });
	Items.Add({ LOCTEXT("Quit", "退出游戏"), [WeakThis]() { if (WeakThis.IsValid()) { UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis.Get(), EQuitPreference::Quit, false); } } });

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	MenuPanel->Present(LOCTEXT("GameTitle", "浮岛搬运行会"), LOCTEXT("Tagline", "大力士一次能推动面前的一整排箱子。\n把每个箱子送上发光的目标点。"), MoveTemp(Items), nullptr);
}

void ASokobanPlayerController::PresentLevelSelect()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	if (!GameMode.IsValid())
	{
		return;
	}

	const TArray<FSokobanLevel>& Levels = GameMode->GetLevels();
	TArray<FSokobanLevelCard> Cards;
	for (int32 Index = 0; Index < Levels.Num(); ++Index)
	{
		const FSokobanLevel& Level = Levels[Index];
		FSokobanLevelCard& Card = Cards.AddDefaulted_GetRef();
		Card.Name = FText::FromString(Level.Name);
		Card.bUnlocked = GameMode->IsLevelUnlocked(Index);
		const int32 Best = GameMode->GetBestMoves(Index);
		Card.bSolved = Best > 0;
		Card.Stars = Card.bSolved ? FSokobanRules::StarsForSolve(Best, Level.Par) : 0;
		if (!Card.bUnlocked)
		{
			Card.Status = LOCTEXT("Locked", "未解锁");
		}
		else if (Card.bSolved)
		{
			Card.Status = FText::Format(LOCTEXT("CardBest", "最佳 {0} 步 · 满星 {1} 步"), Best, Level.Par);
		}
		else
		{
			Card.Status = FText::Format(LOCTEXT("CardOpen", "未完成 · 满星 {0} 步"), Level.Par);
		}
	}

	LevelSelect->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	LevelSelect->Present(FText::FromString(GameMode->GetPackName()), Cards, GameMode->GetContinueIndex(),
		[GameMode](int32 Index) { if (GameMode.IsValid()) { GameMode->StartLevel(Index); } },
		[GameMode]() { if (GameMode.IsValid()) { GameMode->ShowMainMenu(); } });
}

void ASokobanPlayerController::PresentPauseMenu()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	if (!GameMode.IsValid())
	{
		return;
	}

	auto Resume = [GameMode]() { if (GameMode.IsValid()) { GameMode->ResumeGame(); } };
	const FSokobanLevel& Level = GameMode->GetPuzzle().GetLevel();
	const ESokobanPlayContext Context = GameMode->GetPlayContext();

	TArray<FSokobanMenuItem> Items;
	Items.Add({ LOCTEXT("Resume", "继续"), Resume });
	Items.Add({ LOCTEXT("Restart", "重新开始"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ResumeGame(); GameMode->RequestRestart(); } } });
	FText Where;
	if (Context == ESokobanPlayContext::Campaign)
	{
		Items.Add({ LOCTEXT("PauseLevels", "选择关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } } });
		Items.Add({ LOCTEXT("PauseEditor", "关卡编辑器"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowEditorHome(); } } });
		Where = FText::Format(LOCTEXT("PausedLevel", "第 {0} 关 · {1}"), GameMode->GetLevelIndex() + 1, FText::FromString(Level.Name));
	}
	else
	{
		Items.Add({ DescribeTestReturn(Context), [GameMode]() { if (GameMode.IsValid()) { GameMode->LeaveTestPlay(); } } });
		Where = FText::Format(LOCTEXT("PausedTest", "试玩 · {0}"), FText::FromString(Level.Name));
	}
	Items.Add({ LOCTEXT("MainMenu", "返回主菜单"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowMainMenu(); } } });

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	MenuPanel->Present(LOCTEXT("Paused", "暂停"), Where, MoveTemp(Items), Resume);
}

void ASokobanPlayerController::PresentResults()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	if (!GameMode.IsValid())
	{
		return;
	}

	const FSokobanCompletion& Result = GameMode->GetLastCompletion();
	const ESokobanPlayContext Context = GameMode->GetPlayContext();
	TArray<FText> Lines;
	Lines.Add(Result.Par > 0
		? FText::Format(LOCTEXT("MovesUsedPar", "用了 {0} 步（满星 {1} 步）"), Result.Moves, Result.Par)
		: FText::Format(LOCTEXT("MovesUsed", "用了 {0} 步"), Result.Moves));
	if (Result.Par > 0 && Result.Stars == 1)
	{
		// The next star within reach.
		Lines.Add(FText::Format(LOCTEXT("TwoStarsAt", "{0} 步内过关可得两颗星"), FSokobanRules::MovesForStars(2, Result.Par)));
	}
	if (Result.bNewBest)
	{
		Lines.Add(LOCTEXT("NewBest", "新纪录！"));
	}
	if (Context == ESokobanPlayContext::EditorTest)
	{
		Lines.Add(LOCTEXT("TestProven", "通关就证明解得开，回到编辑器就能保存"));
	}

	const FText ReplayLabel = LOCTEXT("Replay", "再玩一次");
	TArray<FSokobanMenuItem> Items;
	TFunction<void()> OnBack;
	if (Context == ESokobanPlayContext::Campaign)
	{
		auto ToLevels = [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } };
		if (Result.bLastLevel)
		{
			Items.Add({ LOCTEXT("BackToLevels", "回到关卡列表"), ToLevels });
		}
		else
		{
			Items.Add({ LOCTEXT("Next", "下一关"), [GameMode]() { if (GameMode.IsValid()) { GameMode->StartNextLevel(); } } });
		}
		Items.Add({ ReplayLabel, [GameMode]() { if (GameMode.IsValid()) { GameMode->StartLevel(GameMode->GetLevelIndex()); } } });
		if (!Result.bLastLevel)
		{
			Items.Add({ LOCTEXT("ResultLevels", "选择关卡"), ToLevels });
		}
		OnBack = ToLevels;
	}
	else
	{
		auto Leave = [GameMode]() { if (GameMode.IsValid()) { GameMode->LeaveTestPlay(); } };
		Items.Add({ DescribeTestReturn(Context), Leave });
		Items.Add({ ReplayLabel, [GameMode]() { if (GameMode.IsValid()) { GameMode->ReplayTestLevel(); } } });
		OnBack = Leave;
	}

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	MenuPanel->Present(Result.bLastLevel ? LOCTEXT("AllClear", "全部完成！") : LOCTEXT("Clear", "过关！"), FText::Join(FText::FromString(TEXT("\n")), Lines), MoveTemp(Items), MoveTemp(OnBack));
	// A drawing nobody has solved yet has no par to rate against.
	if (Context == ESokobanPlayContext::Campaign || Result.Par > 0)
	{
		MenuPanel->ShowStars(Result.Stars);
	}
}

void ASokobanPlayerController::PresentEditorHome()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	if (!GameMode.IsValid())
	{
		return;
	}

	const int32 SavedCount = SokobanShelf::List(SokobanShelf::GetDir(ESokobanShelf::Saved)).Num();
	const int32 DraftCount = SokobanShelf::List(SokobanShelf::GetDir(ESokobanShelf::Drafts)).Num();
	auto ToMainMenu = [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowMainMenu(); } };

	TArray<FSokobanMenuItem> Items;
	Items.Add({ LOCTEXT("DesignNew", "设计新关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->DesignNewLevel(); } } });
	Items.Add({ LOCTEXT("ContinueDesign", "继续设计关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ContinueDesign(); } }, GameMode->CanContinueDesign() });
	Items.Add({ FText::Format(LOCTEXT("ViewSaved", "查看保存的关卡（{0}）"), SavedCount), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowShelf(ESokobanShelf::Saved); } } });
	Items.Add({ FText::Format(LOCTEXT("ViewDrafts", "查看草稿（{0}）"), DraftCount), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowShelf(ESokobanShelf::Drafts); } } });
	Items.Add({ LOCTEXT("MainMenu", "返回主菜单"), ToMainMenu });

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	MenuPanel->Present(LOCTEXT("EditorHomeTitle", "关卡编辑器"),
		LOCTEXT("EditorHomeMessage", "摆好墙、箱子、目标点和大橘，\n确认解得开就能保存成关卡。"), MoveTemp(Items), ToMainMenu);
}

void ASokobanPlayerController::PresentShelf()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	TWeakObjectPtr<ASokobanPlayerController> WeakThis = this;
	if (!GameMode.IsValid())
	{
		return;
	}

	const ESokobanShelf Shelf = GameMode->GetShelf();
	const bool bSaved = Shelf == ESokobanShelf::Saved;
	TArray<FSokobanShelfRow> Rows;
	for (const FSokobanShelfEntry& Entry : SokobanShelf::List(SokobanShelf::GetDir(Shelf)))
	{
		const FSokobanLevelData Data = Entry.Data;
		const FSokobanEditCanvas Canvas = FSokobanEditCanvas::FromRows(Data.Map);
		const FText Size = FText::Format(LOCTEXT("ShelfSize", "{0} × {1}"), Canvas.GetSize().X, Canvas.GetSize().Y);

		FSokobanShelfRow& Row = Rows.AddDefaulted_GetRef();
		Row.Name = FText::FromString(Data.Name);
		if (bSaved)
		{
			Row.Info = FText::Format(LOCTEXT("SavedInfo", "{0} · 满星 {1} 步 · {2}"), Size, Data.Par, DescribeModified(Entry.Modified));
			Row.Actions.Add({ LOCTEXT("ShelfTest", "试玩"), [GameMode, Data]() { if (GameMode.IsValid()) { GameMode->TestPlayShelfLevel(Data); } } });
			Row.Actions.Add({ LOCTEXT("ShelfEdit", "编辑"), [GameMode, Data]() { if (GameMode.IsValid()) { GameMode->EditShelfLevel(ESokobanShelf::Saved, Data); } } });
		}
		else
		{
			Row.Info = FText::Format(LOCTEXT("DraftInfo", "{0} · {1} · {2}"), Size, DescribeDraft(Canvas), DescribeModified(Entry.Modified));
			Row.Actions.Add({ LOCTEXT("ShelfContinue", "继续编辑"), [GameMode, Data]() { if (GameMode.IsValid()) { GameMode->EditShelfLevel(ESokobanShelf::Drafts, Data); } } });
		}
		Row.Actions.Add({ LOCTEXT("ShelfDelete", "删除"), [GameMode, WeakThis, Shelf, Data]()
		{
			if (!GameMode.IsValid() || !WeakThis.IsValid())
			{
				return;
			}
			auto BackToList = [WeakThis]() { if (WeakThis.IsValid()) { WeakThis->PresentShelf(); } };
			WeakThis->PresentConfirm(FText::Format(LOCTEXT("DeleteTitle", "删除「{0}」？"), FText::FromString(Data.Name)),
				LOCTEXT("DeleteMessage", "删除后就找不回来了。"), LOCTEXT("DeleteConfirm", "删除"),
				[GameMode, Shelf, Id = Data.Id, BackToList]() { if (GameMode.IsValid()) { GameMode->DeleteShelfLevel(Shelf, Id); } BackToList(); },
				BackToList);
		} });
	}

	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	ShelfScreen->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	ShelfScreen->Present(bSaved ? LOCTEXT("SavedTitle", "保存的关卡") : LOCTEXT("DraftsTitle", "草稿"),
		bSaved ? LOCTEXT("SavedEmpty", "还没有保存的关卡。\n在编辑器里画好一关，确认解得开就能保存。") : LOCTEXT("DraftsEmpty", "还没有草稿。\n离开编辑器时，没保存的改动会自动存成草稿。"),
		MoveTemp(Rows), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowEditorHome(); } });
}

void ASokobanPlayerController::PresentDrawing()
{
	EditorScreen->Refresh();
	FocusGame();
}

void ASokobanPlayerController::PresentConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, TFunction<void()> OnConfirm, TFunction<void()> OnCancel)
{
	// Cancel comes first and takes the focus, so a stray Enter doesn't confirm.
	TArray<FSokobanMenuItem> Items;
	Items.Add({ LOCTEXT("Cancel", "取消"), OnCancel });
	Items.Add({ ConfirmLabel, MoveTemp(OnConfirm) });

	LevelSelect->SetVisibility(ESlateVisibility::Collapsed);
	ShelfScreen->SetVisibility(ESlateVisibility::Collapsed);
	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI();
	MenuPanel->Present(Title, Message, MoveTemp(Items), MoveTemp(OnCancel));
}

void ASokobanPlayerController::FocusUI()
{
	// No widget to focus here: the input mode would focus it at the end of the frame, taking the focus back
	// from the button the screen focuses as it is presented.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASokobanPlayerController::FocusGame()
{
	// The keyboard goes to the game, and the cursor stays up for the HUD's pause button and the editor's tools.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASokobanPlayerController::SokobanSolve(float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		ASokobanGameMode* GameMode = GetSokobanGameMode();
		if (!GameMode)
		{
			return;
		}

		const FSokobanPuzzle& Puzzle = GameMode->GetPuzzle();
		const FSokobanSolveResult Result = FSokobanSolver::Solve(Puzzle.GetLevel().Layout, Puzzle.GetState(), Puzzle.PushStrength);
		if (!Result.bSolved)
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanSolve: no solution from the current position (%d states explored)."), Result.ExploredStates);
			return;
		}

		UE_LOG(LogSokoban, Log, TEXT("SokobanSolve: playing a %d-move solution."), Result.Moves.Num());
		AutoMoves = Result.Moves;
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanShot(float Delay, bool bQuit, const FString& Name)
{
	FTimerHandle ShotTimer;
	GetWorldTimerManager().SetTimer(ShotTimer, FTimerDelegate::CreateWeakLambda(this, [this, bQuit, Name]()
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogSokoban, Display, TEXT("Screenshot requested: %s"), *Path);

		if (bQuit)
		{
			FTimerHandle QuitTimer;
			GetWorldTimerManager().SetTimer(QuitTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ConsoleCommand(TEXT("quit")); }), 1.5f, false);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanLevel(int32 Index, float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Index]()
	{
		if (ASokobanGameMode* GameMode = GetSokobanGameMode())
		{
			GameMode->StartLevel(Index);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanScreen(const FString& Screen, float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Screen]()
	{
		ASokobanGameMode* GameMode = GetSokobanGameMode();
		if (!GameMode)
		{
			return;
		}

		if (Screen == TEXT("menu"))
		{
			GameMode->ShowMainMenu();
		}
		else if (Screen == TEXT("levels"))
		{
			GameMode->ShowLevelSelect();
		}
		else if (Screen == TEXT("pause"))
		{
			GameMode->PauseGame();
		}
		else if (Screen == TEXT("editor"))
		{
			GameMode->ShowEditorHome();
		}
		else if (Screen == TEXT("saved"))
		{
			GameMode->ShowShelf(ESokobanShelf::Saved);
		}
		else if (Screen == TEXT("drafts"))
		{
			GameMode->ShowShelf(ESokobanShelf::Drafts);
		}
		else if (Screen == TEXT("new"))
		{
			GameMode->DesignNewLevel();
		}
		else if (Screen == TEXT("continue"))
		{
			GameMode->ContinueDesign();
		}
		else if (Screen == TEXT("testsaved"))
		{
			const TArray<FSokobanShelfEntry> Saved = SokobanShelf::List(SokobanShelf::GetDir(ESokobanShelf::Saved));
			if (Saved.Num() > 0)
			{
				GameMode->TestPlayShelfLevel(Saved[0].Data);
			}
		}
		else
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanScreen: unknown screen '%s'."), *Screen);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanPick(int32 Index, float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Index]()
	{
		const bool bPicked = MenuPanel->IsVisible() ? MenuPanel->ActivateButton(Index) : ShelfScreen->IsVisible() && ShelfScreen->ActivateButton(Index);
		if (!bPicked)
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanPick: no button %d to click."), Index);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanEdit(const FString& Brush, int32 X, int32 Y, int32 X2, int32 Y2, float Delay)
{
	ESokobanBrush Paint = ESokobanBrush::Floor;
	if (!ParseBrush(Brush, Paint))
	{
		return;
	}

	const FIntPoint From(X, Y);
	const FIntPoint To = X2 < 0 ? From : FIntPoint(X2, Y2);
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Paint, From, To]()
	{
		if (ASokobanLevelEditor* Editor = GetDrawingEditor())
		{
			Editor->PaintRect(From, To, Paint);
		}
		else
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanEdit: not drawing a level."));
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanEditAt(const FString& Brush, float X, float Y, float Delay)
{
	ESokobanBrush Paint = ESokobanBrush::Floor;
	if (!ParseBrush(Brush, Paint))
	{
		return;
	}

	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Paint, X, Y]()
	{
		ASokobanLevelEditor* Editor = GetDrawingEditor();
		const TOptional<FIntPoint> Cell = Editor ? Editor->CellAtScreen(FVector2D(X, Y)) : TOptional<FIntPoint>();
		UE_LOG(LogSokoban, Log, TEXT("SokobanEditAt: (%.0f, %.0f) is cell %s"), X, Y, Cell.IsSet() ? *Cell->ToString() : TEXT("none"));
		if (Cell.IsSet())
		{
			Editor->PaintRect(Cell.GetValue(), Cell.GetValue(), Paint);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanEditAction(const FString& Action, float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Action]()
	{
		ASokobanGameMode* GameMode = GetSokobanGameMode();
		ASokobanLevelEditor* Editor = GetDrawingEditor();
		if (!GameMode || !Editor)
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanEditAction: not drawing a level."));
			return;
		}

		const TMap<FString, TFunction<void()>> Actions = {
			{ TEXT("undo"), [Editor]() { Editor->Undo(); } },
			{ TEXT("redo"), [Editor]() { Editor->Redo(); } },
			{ TEXT("wallin"), [Editor]() { Editor->WallIn(); } },
			{ TEXT("check"), [Editor]() { Editor->CheckSolvable(); } },
			{ TEXT("test"), [GameMode]() { GameMode->TestPlayDrawing(); } },
			{ TEXT("save"), [Editor]() { Editor->SaveLevel(); } },
			{ TEXT("draft"), [Editor]() { Editor->SaveDraft(); } },
			{ TEXT("back"), [GameMode]() { GameMode->ShowEditorHome(); } },
			{ TEXT("wider"), [Editor]() { Editor->Resize(FIntPoint(1, 0)); } },
			{ TEXT("narrower"), [Editor]() { Editor->Resize(FIntPoint(-1, 0)); } },
			{ TEXT("taller"), [Editor]() { Editor->Resize(FIntPoint(0, 1)); } },
			{ TEXT("shorter"), [Editor]() { Editor->Resize(FIntPoint(0, -1)); } } };
		if (const TFunction<void()>* Run = Actions.Find(Action.ToLower()))
		{
			(*Run)();
		}
		else
		{
			UE_LOG(LogSokoban, Warning, TEXT("SokobanEditAction: unknown action '%s'."), *Action);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanResetProgress()
{
	if (USokobanProgressSubsystem* Progress = GetGameInstance() ? GetGameInstance()->GetSubsystem<USokobanProgressSubsystem>() : nullptr)
	{
		Progress->ResetProgress();
		UE_LOG(LogSokoban, Log, TEXT("Progress reset."));
	}
}

void ASokobanPlayerController::SokobanDump(float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (const ASokobanGameMode* GameMode = GetSokobanGameMode())
		{
			UE_LOG(LogSokoban, Log, TEXT("Dump at %.2fs, move %d"), GetWorld()->GetTimeSeconds(), GameMode->GetPuzzle().GetState().MoveCount);
			GameMode->GetBoardView()->DumpPieces(GameMode->GetPuzzle().GetState());
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanKey(const FString& Direction, bool bDown, float Delay)
{
	static const TMap<FString, ESokobanDirection> Names = {
		{ TEXT("N"), ESokobanDirection::North }, { TEXT("S"), ESokobanDirection::South },
		{ TEXT("E"), ESokobanDirection::East }, { TEXT("W"), ESokobanDirection::West } };
	const ESokobanDirection* Found = Names.Find(Direction.ToUpper());
	if (!Found)
	{
		UE_LOG(LogSokoban, Warning, TEXT("SokobanKey: unknown direction '%s' (use N, S, E or W)."), *Direction);
		return;
	}

	const ESokobanDirection Key = *Found;
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Key, bDown]()
	{
		if (bDown)
		{
			OnDirectionPressed(Key);
		}
		else
		{
			OnDirectionReleased(Key);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

void ASokobanPlayerController::SokobanZoom(float Steps, float Delay)
{
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Steps]()
	{
		ASokobanGameMode* GameMode = GetSokobanGameMode();
		if (GameMode && GameMode->GetCameraRig())
		{
			GameMode->GetCameraRig()->AddZoom(Steps);
		}
	}), FMath::Max(Delay, 0.01f), false);
}

#undef LOCTEXT_NAMESPACE
