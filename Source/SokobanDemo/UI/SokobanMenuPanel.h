// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SokobanUIStyle.h"
#include "SokobanMenuPanel.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class USokobanButton;
class UTextBlock;
class UVerticalBox;

/** One button of a menu panel. */
struct FSokobanMenuItem
{
	FText Label;
	TFunction<void()> OnActivated;
	bool bEnabled = true;
	ESokobanButtonTone Tone = ESokobanButtonTone::Paper;
};

/**
 * Centred paper panel with its title on a ribbon, an optional row of rating stars, an optional message and a column
 * of buttons: pause, results, the editor's home.
 */
UCLASS()
class SOKOBANDEMO_API USokobanMenuPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the panel (without the rating row). OnBack runs on Esc / gamepad B and may be empty. */
	void Present(const FText& Title, const FText& Message, TArray<FSokobanMenuItem> Items, TFunction<void()> OnBack);

	/**
	 * Shows the rating row under the title: Earned of the stars slam into their slots one after another,
	 * each landing with a bounce, a glow, a burst of sparks and a jolt of the panel. Call after Present.
	 */
	void ShowStars(int32 Earned);

	/** Gives keyboard / gamepad focus to the first enabled button. */
	void FocusFirstButton();

	/** Dev: runs the Index-th button as if clicked. Returns false when there is no such enabled button. */
	bool ActivateButton(int32 Index);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** Poses the stars, glows and sparks, and jolts the panel, StarClock seconds into the reveal. */
	void AnimateStars();

	/** The panel with its ribbon, jolted as the stars land. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> PanelFrame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> StarRow;

	/** The earned look of each star, the glow behind it, and its sparks (SparksPerStar per star, in star order). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> StarImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> GlowImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SparkImages;

	int32 StarsEarned = 0;

	/** How many stars have landed with their chime so far. */
	int32 StarsChimed = 0;

	/** Seconds since ShowStars; negative when no reveal is running. */
	float StarClock = -1.f;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ButtonColumn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> Buttons;

	TFunction<void()> BackAction;
};
