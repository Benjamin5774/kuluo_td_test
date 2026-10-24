// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "UI/SokobanUIStyle.h"
#include "SokobanButton.generated.h"

/** Button with a native callback, a colour of its own, and a visible keyboard / gamepad focus state. */
UCLASS()
class SOKOBANDEMO_API USokobanButton : public UButton
{
	GENERATED_BODY()

public:
	/** Runs on click, Enter / Space, or the gamepad accept button. */
	TFunction<void()> OnActivated;

	/** The colour at rest, before the button is first shown. */
	void SetTone(ESokobanButtonTone InTone);

	/** Shows the highlighted look while the button holds keyboard or gamepad focus. Call every frame. */
	void UpdateFocusLook();

	/** Shows the highlighted look for a choice that stays made (the editor's brush), focus or not. */
	void ShowSelected(bool bSelected);

	/** Before the button is first shown: whether it can take keyboard / gamepad focus (it can by default). */
	void InitFocusable(bool bInFocusable) { InitIsFocusable(bInFocusable); }

protected:
	virtual void SynchronizeProperties() override;

private:
	UFUNCTION()
	void HandleClicked();

	void ApplyLook(bool bHighlighted);

	ESokobanButtonTone Tone = ESokobanButtonTone::Paper;
	bool bShowingFocus = false;
	bool bSelected = false;
};
