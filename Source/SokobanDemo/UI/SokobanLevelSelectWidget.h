// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SokobanLevelSelectWidget.generated.h"

class USokobanButton;
class UTextBlock;
class UUniformGridPanel;

/** What one level card shows. */
struct FSokobanLevelCard
{
	FText Name;
	FText Status;
	bool bUnlocked = false;
	bool bSolved = false;
};

/** Grid of level cards with lock / best-score status. */
UCLASS()
class SOKOBANDEMO_API USokobanLevelSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the grid. OnChosen receives the level index; OnBack runs on the back button, Esc or gamepad B. */
	void Present(const FText& PackName, const TArray<FSokobanLevelCard>& Cards, int32 FocusIndex, TFunction<void(int32)> OnChosen, TFunction<void()> OnBack);

	void FocusPreferredCard();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** Cards per row. */
	static constexpr int32 Columns = 3;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PackText;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> Grid;

	UPROPERTY(Transient)
	TObjectPtr<USokobanButton> BackButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USokobanButton>> CardButtons;

	TFunction<void()> BackAction;
	int32 PreferredFocus = 0;
};
