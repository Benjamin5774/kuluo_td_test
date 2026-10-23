// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UTextBlock;
class UWidgetTree;

/** Shared look of the game's UI, built in code so no widget assets are needed. */
namespace SokobanStyle
{
	inline const FLinearColor Text(0.96f, 0.93f, 0.86f);
	inline const FLinearColor TextDim(0.62f, 0.6f, 0.56f);
	inline const FLinearColor Accent(1.f, 0.7f, 0.18f);
	inline const FLinearColor Panel(0.03f, 0.035f, 0.05f, 0.9f);
	inline const FLinearColor Dim(0.f, 0.f, 0.f, 0.45f);
	inline const FLinearColor ButtonNormal(0.12f, 0.13f, 0.17f, 1.f);
	inline const FLinearColor ButtonHovered(0.8f, 0.5f, 0.1f, 1.f);
	inline const FLinearColor ButtonPressed(0.6f, 0.36f, 0.06f, 1.f);
	inline const FLinearColor ButtonDisabled(0.07f, 0.07f, 0.09f, 0.7f);

	/** The engine's Roboto, whose fallback face covers Chinese. */
	FSlateFontInfo Font(int32 Size, bool bBold = false);

	FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius);

	/** Button look; bFocused swaps in the hovered look so keyboard and gamepad users can see the selection. */
	FButtonStyle ButtonStyle(bool bFocused = false);

	/** Text block in the shared style, with a soft shadow for readability over the 3D scene. */
	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Content, int32 Size, bool bBold = false, const FLinearColor& Color = SokobanStyle::Text);
}
