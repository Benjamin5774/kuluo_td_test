// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanMenuPanel.generated.h"

class USokobanButton;
class UTextBlock;
class UVerticalBox;

/** One button of a menu panel. */
struct FSokobanMenuItem
{
	FText Label;
	TFunction<void()> OnActivated;
	bool bEnabled = true;
};

/** Centred panel with a title, an optional message and a column of buttons: main menu, pause, results. */
UCLASS()
class SOKOBANDEMO_API USokobanMenuPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the panel. OnBack runs on Esc / gamepad B and may be empty. */
	void Present(const FText& Title, const FText& Message, TArray<FSokobanMenuItem> Items, TFunction<void()> OnBack);

	/** Gives keyboard / gamepad focus to the first enabled button. */
	void FocusFirstButton();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ButtonColumn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> Buttons;

	TFunction<void()> BackAction;
};
