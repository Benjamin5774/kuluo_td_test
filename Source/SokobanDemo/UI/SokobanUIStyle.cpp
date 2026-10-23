// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"

FSlateFontInfo SokobanStyle::Font(int32 Size, bool bBold)
{
	static TWeakObjectPtr<const UFont> Roboto;
	if (!Roboto.IsValid())
	{
		Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	}
	return FSlateFontInfo(Roboto.Get(), Size, bBold ? TEXT("Bold") : TEXT("Regular"));
}

FSlateBrush SokobanStyle::RoundedBrush(const FLinearColor& Color, float Radius)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Color);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(FLinearColor::Transparent), 0.f);
	return Brush;
}

FButtonStyle SokobanStyle::ButtonStyle(bool bFocused)
{
	FButtonStyle Style;
	Style.SetNormal(RoundedBrush(bFocused ? ButtonHovered : ButtonNormal, 10.f));
	Style.SetHovered(RoundedBrush(ButtonHovered, 10.f));
	Style.SetPressed(RoundedBrush(ButtonPressed, 10.f));
	Style.SetDisabled(RoundedBrush(ButtonDisabled, 10.f));
	Style.SetNormalPadding(FMargin(24.f, 10.f));
	Style.SetPressedPadding(FMargin(24.f, 11.f, 24.f, 9.f));
	return Style;
}

UTextBlock* SokobanStyle::MakeText(UWidgetTree& Tree, const FText& Content, int32 Size, bool bBold, const FLinearColor& Color)
{
	UTextBlock* Block = Tree.ConstructWidget<UTextBlock>();
	Block->SetText(Content);
	Block->SetFont(Font(Size, bBold));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	return Block;
}
