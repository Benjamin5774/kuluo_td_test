// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanButton.h"
#include "Game/SokobanAudioSubsystem.h"

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

void USokobanButton::SetTone(ESokobanButtonTone InTone)
{
	Tone = InTone;
	ApplyLook(bShowingFocus || bSelected);
}

void USokobanButton::UpdateFocusLook()
{
	const bool bFocused = HasKeyboardFocus();
	if (bFocused != bShowingFocus)
	{
		bShowingFocus = bFocused;
		ApplyLook(bShowingFocus || bSelected);
	}
}

void USokobanButton::ShowSelected(bool bInSelected)
{
	bSelected = bInSelected;
	ApplyLook(bShowingFocus || bSelected);
}

void USokobanButton::ApplyLook(bool bHighlighted)
{
	SetStyle(SokobanStyle::ButtonStyle(bHighlighted, Tone));
	// The focused button stands out a little, as if lifted towards the player.
	SetRenderScale(FVector2D(bShowingFocus ? 1.04f : 1.f));
}
