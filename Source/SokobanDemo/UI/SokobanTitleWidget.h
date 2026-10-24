// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SokobanMenuPanel.h"
#include "SokobanTitleWidget.generated.h"

class UImage;
class USokobanButton;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * The title screen over the title scene: the game's name, whose letters drop in one after another and then bob
 * like boats, a subtitle, and the main menu sliding in from the left, a paw pointing at the chosen button.
 */
UCLASS()
class SOKOBANDEMO_API USokobanTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the menu. With bIntro everything arrives anew; otherwise it is simply there. */
	void Present(TArray<FSokobanMenuItem> Items, bool bIntro);

	/** Gives keyboard / gamepad focus to the first enabled button. */
	void FocusFirstButton();

	/** Dev: runs the Index-th button as if clicked. Returns false when there is no such enabled button. */
	bool ActivateButton(int32 Index);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Poses the letters, subtitle and buttons Clock seconds into the intro. */
	void Animate();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> Letters;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Subtitle;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ButtonColumn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> Buttons;

	/** Per button: its row (which slides in), and the paw beside it, shown while the button is focused or hovered. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> Rows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Paws;

	/** Seconds since the intro began. */
	float Clock = 0.f;

	/** How many letters have touched down with a knock so far. */
	int32 LettersLanded = 0;
};
