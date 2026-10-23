// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanHUDWidget.generated.h"

class UTextBlock;

/** In-game overlay: level title, move counter and control hints. */
UCLASS()
class SOKOBANDEMO_API USokobanHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLevelTitle(const FText& Title);
	void SetStats(const FText& Stats);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;
};
