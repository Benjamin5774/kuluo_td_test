// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SokobanMenuPanel.h"
#include "SokobanShelfWidget.generated.h"

class UScrollBox;
class USokobanButton;
class UTextBlock;

/** One level in a shelf list: its name, a line about it, and what can be done with it. */
struct FSokobanShelfRow
{
	FText Name;
	FText Info;
	TArray<FSokobanMenuItem> Actions;
};

/** A scrolling list of levels made in the editor (saved levels or drafts), each with its own action buttons. */
UCLASS()
class SOKOBANDEMO_API USokobanShelfWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the list; EmptyText shows when there are no rows. OnBack runs on the back button, Esc or gamepad B. */
	void Present(const FText& Title, const FText& EmptyText, TArray<FSokobanShelfRow> Rows, TFunction<void()> OnBack);

	/** Gives keyboard / gamepad focus to the first button. */
	void FocusFirstButton();

	/** Dev: runs the Index-th row button (counting along each row, then down) as if clicked. */
	bool ActivateButton(int32 Index);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EmptyMessage;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> List;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> BackButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> Buttons;

	TFunction<void()> BackAction;
};
