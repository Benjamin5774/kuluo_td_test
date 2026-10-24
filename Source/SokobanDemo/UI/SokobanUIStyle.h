// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;
class UWidget;
class UWidgetTree;

/** How heavy a face: body text, then labels and buttons, then titles. */
enum class ESokobanFontWeight : uint8
{
	Medium,
	Bold,
	Heavy
};

/** A button's colour at rest; whatever its tone, a button turns gold while focused, hovered or selected. */
enum class ESokobanButtonTone : uint8
{
	Paper,
	Sea,
	Coral
};

/**
 * Shared look of the game's UI, built in code so no widget assets are needed: chunky cartoon shapes with thick ink
 * outlines on paper (textures drawn by the agent tool make_ui_textures.py), set in Haidao Sans, the game's subset of
 * Source Han Sans (make_ui_fonts.py).
 */
namespace SokobanStyle
{
	/** Outlines, and text on paper. */
	inline const FLinearColor Ink(FColor(27, 33, 64));
	inline const FLinearColor InkSoft(FColor(104, 106, 132));
	/** Panels, and text over the 3D scene. */
	inline const FLinearColor Paper(FColor(255, 248, 232));
	inline const FLinearColor Gold(FColor(255, 196, 58));
	inline const FLinearColor GoldLight(FColor(255, 216, 108));
	inline const FLinearColor Sea(FColor(52, 172, 214));
	inline const FLinearColor SeaLight(FColor(126, 208, 238));
	inline const FLinearColor Coral(FColor(255, 128, 108));
	inline const FLinearColor Grey(FColor(208, 202, 190));
	/** Problems, in a status line. */
	inline const FLinearColor Warning(FColor(212, 66, 46));

	/** Haidao Sans; anything it lacks (a rare character typed into a level name) comes from DroidSansFallback. */
	FSlateFontInfo Font(int32 Size, ESokobanFontWeight Weight = ESokobanFontWeight::Medium);

	/** The same with an ink outline Outline pixels thick, for letters over the 3D scene. */
	FSlateFontInfo OutlinedFont(int32 Size, ESokobanFontWeight Weight, int32 Outline);

	/** Text on paper (panels, buttons, cards): ink unless told otherwise, no shadow. */
	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Content, int32 Size, ESokobanFontWeight Weight = ESokobanFontWeight::Medium, const FLinearColor& Color = SokobanStyle::Ink);

	/** Text over the 3D scene: paper-coloured unless told otherwise, with an ink outline and a soft shadow. */
	UTextBlock* MakeSceneText(UWidgetTree& Tree, const FText& Content, int32 Size, ESokobanFontWeight Weight = ESokobanFontWeight::Bold, const FLinearColor& Color = SokobanStyle::Paper);

	/** A plain rounded fill. */
	FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius);

	/** The paper panel, its shadow baked in. */
	FSlateBrush PanelBrush();

	/** The sea-blue ribbon that carries a panel's title. */
	FSlateBrush RibbonBrush();

	/** A list row (a card with a thin lip) and a pill (HUD chips, key caps, text fields), their bodies tinted Fill. */
	FSlateBrush CardBrush(const FLinearColor& Fill);
	FSlateBrush PillBrush(const FLinearColor& Fill);

	/** A disc Size pixels across, tinted Fill (level medallions). */
	FSlateBrush CircleBrush(const FLinearColor& Fill, float Size);

	/** One of the icons, T_Icon<Name> or T_Brush<Name>, Size pixels across. */
	FSlateBrush IconBrush(const TCHAR* Name, float Size);

	/** Darkens the screen towards its edges behind a menu. */
	FSlateBrush VignetteBrush();

	/** Button look: gold when bFocused (keyboard / gamepad focus, or the selected brush), else its Tone. */
	FButtonStyle ButtonStyle(bool bFocused = false, ESokobanButtonTone Tone = ESokobanButtonTone::Paper);

	/** A paper panel with Padding inside. */
	UBorder* MakePanel(UWidgetTree& Tree, const FMargin& Padding);

	/** A ribbon with Title on it in heavy white letters; OutText, when given, receives the text block. */
	UWidget* MakeRibbon(UWidgetTree& Tree, const FText& Title, int32 Size, UTextBlock** OutText = nullptr);

	/** Adds a key cap ("Z", "Esc") and what it does to Row, for control hints; bOnDark for light lettering. */
	void AddKeyHint(UWidgetTree& Tree, UHorizontalBox& Row, const FText& Key, const FText& Action, bool bOnDark);

	/** A rating star Size pixels across: earned (gold) or an empty slot. */
	FSlateBrush StarBrush(bool bEarned, float Size);

	/** A soft round glow Size pixels across, flashed behind a star as it lands. */
	FSlateBrush GlowBrush(float Size, const FLinearColor& Tint);
}
