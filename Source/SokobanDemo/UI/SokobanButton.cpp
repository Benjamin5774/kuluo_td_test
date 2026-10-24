// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanButton.h"
#include "Game/SokobanAudioSubsystem.h"
#include "UI/SokobanUIStyle.h"

void USokobanButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	OnClicked.AddUniqueDynamic(this, &USokobanButton::HandleClicked);
}

void USokobanButton::HandleClicked()
{
	if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
	{
		Audio->Play(ESokobanSound::Click, 0.5f);
	}
	if (OnActivated)
	{
		OnActivated();
	}
}

void USokobanButton::UpdateFocusLook()
{
	const bool bFocused = HasKeyboardFocus();
	if (bFocused != bShowingFocus)
	{
		bShowingFocus = bFocused;
		SetStyle(SokobanStyle::ButtonStyle(bFocused));
	}
}
