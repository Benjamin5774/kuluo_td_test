// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "SokobanButton.generated.h"

/** Button with a native callback and a visible keyboard / gamepad focus state. */
UCLASS()
class SOKOBANDEMO_API USokobanButton : public UButton
{
	GENERATED_BODY()

public:
	/** Runs on click, Enter / Space, or the gamepad accept button. */
	TFunction<void()> OnActivated;

	/** Shows the highlighted look while the button holds keyboard or gamepad focus. Call every frame. */
	void UpdateFocusLook();

protected:
	virtual void SynchronizeProperties() override;

private:
	UFUNCTION()
	void HandleClicked();

	bool bShowingFocus = false;
};
