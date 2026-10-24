// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanHUDWidget.generated.h"

class USokobanButton;
class UTextBlock;

/** In-game overlay: level title, move counter, control hints and a pause button. */
UCLASS()
class SOKOBANDEMO_API USokobanHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLevelTitle(const FText& Title);
	void SetStats(const FText& Stats);

	/** Runs when the pause button is clicked. */
	TFunction<void()> OnPause;

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> PauseButton;
};
