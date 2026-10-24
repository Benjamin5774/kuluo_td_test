// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanEditorWidget.generated.h"

class ASokobanLevelEditor;
class UEditableTextBox;
class UPanelWidget;
class USokobanButton;
class UTextBlock;

/**
 * The level editor's screen around the board: the level's name and size at the top left, the brushes down the
 * left side, actions (undo, redo, wall in, check, test play, save, draft, back) down the right, and a status
 * line with the controls at the bottom. Its buttons never take keyboard focus, so shortcuts keep working.
 */
UCLASS()
class SOKOBANDEMO_API USokobanEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows and controls this editor session; refreshes whenever it changes. */
	void Bind(ASokobanLevelEditor* InEditor);

	/** Flow actions the screen can't take by itself. */
	TFunction<void()> OnTestPlay;
	TFunction<void()> OnBack;

	/** Re-reads everything from the editor. */
	void Refresh();

	/** The name box has the keyboard: keys are typing, not shortcuts. */
	bool IsEditingName() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	/** A button in a tool column: Label, at least Width wide, running Action. */
	USokobanButton* AddToolButton(UPanelWidget* Column, const FText& Label, float Width, TFunction<void()> Action);

	UFUNCTION()
	void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Keeps clicks on a panel (between its buttons) from reaching the board and painting. */
	UFUNCTION()
	FEventReply HandlePanelMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	TWeakObjectPtr<ASokobanLevelEditor> Editor;
	FDelegateHandle ChangedHandle;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> NameBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SizeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	/** One per brush, in ESokobanBrush order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> BrushButtons;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> UndoButton;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> RedoButton;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> TestButton;
};
