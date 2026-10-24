// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanHUDWidget.generated.h"

class UBorder;
class USokobanButton;
class UTextBlock;

/** In-game overlay: the level's badge and name, chips for moves, par and best, control hints and a pause button. */
UCLASS()
class SOKOBANDEMO_API USokobanHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Badge: what the medallion shows (the level's number, or 试玩); Name: the level's name. */
	void SetLevel(const FText& Badge, const FText& Name);

	/** Moves made so far, and the level's par and best (a chip is hidden while its number is 0). */
	void SetStats(int32 Moves, int32 Par, int32 Best);

	/** Runs when the pause button is clicked. */
	TFunction<void()> OnPause;

protected:
	virtual void NativeOnInitialized() override;

private:
	/** Adds a chip (icon and text) to Row; returns the chip, whose text is OutText. */
	UBorder* AddChip(class UHorizontalBox* Row, const FSlateBrush& Icon, UTextBlock*& OutText);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BadgeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MovesText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ParText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BestText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ParChip;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BestChip;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> PauseButton;
};
