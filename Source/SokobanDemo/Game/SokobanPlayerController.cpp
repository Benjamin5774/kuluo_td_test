// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanPlayerController.h"
#include "SokobanDemo.h"
#include "Game/SokobanGameMode.h"
#include "Game/SokobanProgressSubsystem.h"
#include "Puzzle/SokobanSolver.h"
#include "UI/SokobanHUDWidget.h"
#include "UI/SokobanLevelSelectWidget.h"
#include "UI/SokobanMenuPanel.h"
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

ASokobanPlayerController::ASokobanPlayerController()
{
	// The camera rig is the view target for the whole session; never swap it for a pawn.
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = false;

	HUDClass = USokobanHUDWidget::StaticClass();
	MenuPanelClass = USokobanMenuPanel::StaticClass();
	LevelSelectClass = USokobanLevelSelectWidget::StaticClass();
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
	}
}

void ASokobanPlayerController::CreateWidgets()
{
	HUD = CreateWidget<USokobanHUDWidget>(this, HUDClass);
	HUD->AddToViewport(0);
	HUD->SetVisibility(ESlateVisibility::Collapsed);

	MenuPanel = CreateWidget<USokobanMenuPanel>(this, MenuPanelClass);
	MenuPanel->AddToViewport(10);
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);

	LevelSelect = CreateWidget<USokobanLevelSelectWidget>(this, LevelSelectClass);
	LevelSelect->AddToViewport(10);
	LevelSelect->SetVisibility(ESlateVisibility::Collapsed);
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
			IssueMove(HeldDirections.Last());
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

	if (IsBoardBusy())
	{
		// Keep the latest press; it plays when the running move lands.
		BufferedMove = Direction;
	}
	else
	{
		IssueMove(Direction);
	}
}

void ASokobanPlayerController::OnDirectionReleased(ESokobanDirection Direction)
{
	HeldDirections.Remove(Direction);
}

void ASokobanPlayerController::OnUndoPressed()
{
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!GameMode || !IsPlaying())
	{
		return;
	}

	AutoMoves.Reset();
	BufferedMove.Reset();
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
		if (GameMode->RequestRestart())
		{
			MarkBoardBusy();
		}
	}
}

void ASokobanPlayerController::OnPausePressed()
{
	if (ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		GameMode->PauseGame();
	}
}

void ASokobanPlayerController::OnZoom(const FInputActionValue& Value)
{
	ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (GameMode && GameMode->GetCameraRig() && IsPlaying())
	{
		GameMode->GetCameraRig()->AddZoom(Value.Get<float>());
	}
}

void ASokobanPlayerController::IssueMove(ESokobanDirection Direction)
{
	if (ASokobanGameMode* GameMode = GetSokobanGameMode())
	{
		GameMode->RequestMove(Direction);
		MarkBoardBusy();
	}
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
	bUndoHeld = false;
	if (State != ESokobanFlowState::Playing)
	{
		AutoMoves.Reset();
	}

	if (!HUD)
	{
		return;
	}

	const bool bInLevel = State == ESokobanFlowState::Playing || State == ESokobanFlowState::Paused || State == ESokobanFlowState::LevelComplete;
	HUD->SetVisibility(bInLevel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	LevelSelect->SetVisibility(ESlateVisibility::Collapsed);

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
	default:
		FocusGame();
		break;
	}
}

void ASokobanPlayerController::HandlePuzzleChanged()
{
	const ASokobanGameMode* GameMode = GetSokobanGameMode();
	if (!HUD || !GameMode || GameMode->GetLevelIndex() == INDEX_NONE)
	{
		return;
	}

	const FSokobanLevel& Level = GameMode->GetPuzzle().GetLevel();
	HUD->SetLevelTitle(FText::Format(LOCTEXT("LevelTitle", "第 {0} 关 · {1}"), GameMode->GetLevelIndex() + 1, FText::FromString(Level.Name)));

	TArray<FText> Stats;
	Stats.Add(FText::Format(LOCTEXT("Moves", "步数 {0}"), GameMode->GetPuzzle().GetState().MoveCount));
	if (Level.Par > 0)
	{
		Stats.Add(FText::Format(LOCTEXT("Par", "标准 {0}"), Level.Par));
	}
	if (const int32 Best = GameMode->GetBestMoves(GameMode->GetLevelIndex()))
	{
		Stats.Add(FText::Format(LOCTEXT("Best", "最佳 {0}"), Best));
	}
	HUD->SetStats(FText::Join(FText::FromString(TEXT("      ")), Stats));
}

void ASokobanPlayerController::PresentMainMenu()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	TWeakObjectPtr<ASokobanPlayerController> WeakThis = this;
	const bool bHasProgress = GameMode.IsValid() && GameMode->HasAnyProgress();

	TArray<FSokobanMenuItem> Items;
	Items.Add({ bHasProgress ? LOCTEXT("Continue", "继续游戏") : LOCTEXT("Start", "开始游戏"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ContinueGame(); } } });
	Items.Add({ LOCTEXT("SelectLevel", "选择关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } } });
	Items.Add({ LOCTEXT("Editor", "关卡编辑器（制作中）"), nullptr, false });
	Items.Add({ LOCTEXT("Quit", "退出游戏"), [WeakThis]() { if (WeakThis.IsValid()) { UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis.Get(), EQuitPreference::Quit, false); } } });

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI(MenuPanel);
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
		if (!Card.bUnlocked)
		{
			Card.Status = LOCTEXT("Locked", "未解锁");
		}
		else if (Card.bSolved)
		{
			Card.Status = FText::Format(LOCTEXT("CardBest", "最佳 {0} 步 · 标准 {1} 步"), Best, Level.Par);
		}
		else
		{
			Card.Status = FText::Format(LOCTEXT("CardOpen", "未完成 · 标准 {0} 步"), Level.Par);
		}
	}

	LevelSelect->SetVisibility(ESlateVisibility::Visible);
	FocusUI(LevelSelect);
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

	TArray<FSokobanMenuItem> Items;
	Items.Add({ LOCTEXT("Resume", "继续"), Resume });
	Items.Add({ LOCTEXT("Restart", "重新开始"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ResumeGame(); GameMode->RequestRestart(); } } });
	Items.Add({ LOCTEXT("PauseLevels", "选择关卡"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } } });
	Items.Add({ LOCTEXT("MainMenu", "返回主菜单"), [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowMainMenu(); } } });

	const FSokobanLevel& Level = GameMode->GetPuzzle().GetLevel();
	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI(MenuPanel);
	MenuPanel->Present(LOCTEXT("Paused", "暂停"), FText::Format(LOCTEXT("PausedLevel", "第 {0} 关 · {1}"), GameMode->GetLevelIndex() + 1, FText::FromString(Level.Name)), MoveTemp(Items), Resume);
}

void ASokobanPlayerController::PresentResults()
{
	TWeakObjectPtr<ASokobanGameMode> GameMode = GetSokobanGameMode();
	if (!GameMode.IsValid())
	{
		return;
	}

	const FSokobanCompletion& Result = GameMode->GetLastCompletion();
	TArray<FText> Lines;
	Lines.Add(Result.Par > 0
		? FText::Format(LOCTEXT("MovesUsedPar", "用了 {0} 步（标准 {1} 步）"), Result.Moves, Result.Par)
		: FText::Format(LOCTEXT("MovesUsed", "用了 {0} 步"), Result.Moves));
	if (Result.Par > 0 && Result.Moves <= Result.Par)
	{
		Lines.Add(LOCTEXT("ParMet", "达到标准步数，漂亮！"));
	}
	if (Result.bNewBest)
	{
		Lines.Add(LOCTEXT("NewBest", "新纪录！"));
	}

	auto ToLevels = [GameMode]() { if (GameMode.IsValid()) { GameMode->ShowLevelSelect(); } };

	TArray<FSokobanMenuItem> Items;
	if (Result.bLastLevel)
	{
		Items.Add({ LOCTEXT("BackToLevels", "回到关卡列表"), ToLevels });
	}
	else
	{
		Items.Add({ LOCTEXT("Next", "下一关"), [GameMode]() { if (GameMode.IsValid()) { GameMode->StartNextLevel(); } } });
	}
	Items.Add({ LOCTEXT("Replay", "再玩一次"), [GameMode]() { if (GameMode.IsValid()) { GameMode->StartLevel(GameMode->GetLevelIndex()); } } });
	if (!Result.bLastLevel)
	{
		Items.Add({ LOCTEXT("ResultLevels", "选择关卡"), ToLevels });
	}

	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	FocusUI(MenuPanel);
	MenuPanel->Present(Result.bLastLevel ? LOCTEXT("AllClear", "全部完成！") : LOCTEXT("Clear", "过关！"), FText::Join(FText::FromString(TEXT("\n")), Lines), MoveTemp(Items), ToLevels);
}

void ASokobanPlayerController::FocusUI(UUserWidget* Widget)
{
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASokobanPlayerController::FocusGame()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
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
