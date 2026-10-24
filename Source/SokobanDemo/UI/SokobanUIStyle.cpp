// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"

namespace
{
	/** An image from /Game/Sokoban/UI (drawn by the agent tools make_star_textures.py and make_ui_textures.py), loaded once. */
	UTexture2D* UITexture(const TCHAR* Name)
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Loaded;
		TWeakObjectPtr<UTexture2D>& Texture = Loaded.FindOrAdd(Name);
		if (!Texture.IsValid())
		{
			Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Sokoban/UI/%s.%s"), Name, Name));
		}
		return Texture.Get();
	}

	FSlateBrush ImageBrush(UTexture2D* Texture, float Size, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Size, Size);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** A 9-sliced frame: the corners (Margin, as fractions of the texture) keep their shape at ImageSize scale. */
	FSlateBrush BoxBrush(const TCHAR* Texture, const FVector2D& ImageSize, const FMargin& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.SetResourceObject(UITexture(Texture));
		Brush.ImageSize = ImageSize;
		Brush.Margin = Margin;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	FSlateBrush ButtonBrush(const TCHAR* Texture, const FLinearColor& Tint)
	{
		return BoxBrush(Texture, FVector2D(64.f, 48.f), FMargin(0.25f, 0.3f, 0.25f, 0.4f), Tint);
	}

	/**
	 * The UI font, put together at first use: the three Haidao Sans faces as its typefaces, and the engine Roboto's
	 * fallback (DroidSansFallback) for characters they don't have. Rooted, as nothing else holds it.
	 */
	const UFont* UIFont()
	{
		static UFont* Font = nullptr;
		if (!Font)
		{
			Font = NewObject<UFont>(GetTransientPackage(), TEXT("SokobanUIFont"));
			Font->AddToRoot();
			Font->FontCacheType = EFontCacheType::Runtime;
			for (const TCHAR* Weight : { TEXT("Medium"), TEXT("Bold"), TEXT("Heavy") })
			{
				const FString Path = FString::Printf(TEXT("/Game/Sokoban/UI/Fonts/FF_HaidaoSans_%s.FF_HaidaoSans_%s"), Weight, Weight);
				if (const UFontFace* Face = LoadObject<UFontFace>(nullptr, *Path))
				{
					FTypefaceEntry& Entry = Font->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
					Entry.Name = Weight;
					Entry.Font = FFontData(Face);
				}
			}
			if (const UFont* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
			{
				Font->CompositeFont.FallbackTypeface = Roboto->CompositeFont.FallbackTypeface;
				// Without our faces (they failed to load), the game still reads: Roboto and its fallback.
				if (Font->CompositeFont.DefaultTypeface.Fonts.Num() == 0)
				{
					Font->CompositeFont.DefaultTypeface = Roboto->CompositeFont.DefaultTypeface;
				}
			}
		}
		return Font;
	}

	FName WeightName(ESokobanFontWeight Weight)
	{
		switch (Weight)
		{
		case ESokobanFontWeight::Bold:
			return TEXT("Bold");
		case ESokobanFontWeight::Heavy:
			return TEXT("Heavy");
		default:
			return TEXT("Medium");
		}
	}
}

FSlateFontInfo SokobanStyle::Font(int32 Size, ESokobanFontWeight Weight)
{
	return FSlateFontInfo(UIFont(), Size, WeightName(Weight));
}

FSlateFontInfo SokobanStyle::OutlinedFont(int32 Size, ESokobanFontWeight Weight, int32 Outline)
{
	FSlateFontInfo Info = Font(Size, Weight);
	Info.OutlineSettings.OutlineSize = Outline;
	Info.OutlineSettings.OutlineColor = Ink;
	return Info;
}

UTextBlock* SokobanStyle::MakeText(UWidgetTree& Tree, const FText& Content, int32 Size, ESokobanFontWeight Weight, const FLinearColor& Color)
{
	UTextBlock* Block = Tree.ConstructWidget<UTextBlock>();
	Block->SetText(Content);
	Block->SetFont(Font(Size, Weight));
	Block->SetColorAndOpacity(FSlateColor(Color));
	return Block;
}

UTextBlock* SokobanStyle::MakeSceneText(UWidgetTree& Tree, const FText& Content, int32 Size, ESokobanFontWeight Weight, const FLinearColor& Color)
{
	UTextBlock* Block = MakeText(Tree, Content, Size, Weight, Color);
	Block->SetFont(OutlinedFont(Size, Weight, Size >= 28 ? 3 : 2));
	Block->SetShadowOffset(FVector2D(0.f, 2.5f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	return Block;
}

FSlateBrush SokobanStyle::RoundedBrush(const FLinearColor& Color, float Radius)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Color);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(FLinearColor::Transparent), 0.f);
	return Brush;
}

FSlateBrush SokobanStyle::PanelBrush()
{
	return BoxBrush(TEXT("T_UIPanel"), FVector2D(128.f, 128.f), FMargin(0.31f, 0.31f, 0.31f, 0.34f), FLinearColor::White);
}

FSlateBrush SokobanStyle::RibbonBrush()
{
	return BoxBrush(TEXT("T_UIRibbon"), FVector2D(300.f, 84.f), FMargin(0.25f, 0.f, 0.25f, 0.f), FLinearColor::White);
}

FSlateBrush SokobanStyle::CardBrush(const FLinearColor& Fill)
{
	return BoxBrush(TEXT("T_UICard"), FVector2D(64.f, 64.f), FMargin(0.3f), Fill);
}

FSlateBrush SokobanStyle::PillBrush(const FLinearColor& Fill)
{
	return BoxBrush(TEXT("T_UIPill"), FVector2D(64.f, 32.f), FMargin(0.45f, 0.45f), Fill);
}

FSlateBrush SokobanStyle::CircleBrush(const FLinearColor& Fill, float Size)
{
	return ImageBrush(UITexture(TEXT("T_UICircle")), Size, Fill);
}

FSlateBrush SokobanStyle::IconBrush(const TCHAR* Name, float Size)
{
	UTexture2D* Texture = UITexture(*FString::Printf(TEXT("T_Icon%s"), Name));
	if (!Texture)
	{
		Texture = UITexture(*FString::Printf(TEXT("T_Brush%s"), Name));
	}
	return ImageBrush(Texture, Size, FLinearColor::White);
}

FSlateBrush SokobanStyle::VignetteBrush()
{
	FSlateBrush Brush = ImageBrush(UITexture(TEXT("T_UIVignette")), 256.f, FLinearColor::White);
	return Brush;
}

FButtonStyle SokobanStyle::ButtonStyle(bool bFocused, ESokobanButtonTone Tone)
{
	const FLinearColor Rest = Tone == ESokobanButtonTone::Sea ? SeaLight : (Tone == ESokobanButtonTone::Coral ? Coral : Paper);
	FButtonStyle Style;
	Style.SetNormal(ButtonBrush(TEXT("T_UIButton"), bFocused ? Gold : Rest));
	Style.SetHovered(ButtonBrush(TEXT("T_UIButton"), bFocused ? GoldLight : Gold));
	Style.SetPressed(ButtonBrush(TEXT("T_UIButtonDown"), Gold));
	Style.SetDisabled(ButtonBrush(TEXT("T_UIButton"), Grey));
	// Room below for the lip; pressed, the face (and its label) sinks into it.
	Style.SetNormalPadding(FMargin(22.f, 8.f, 22.f, 13.f));
	Style.SetPressedPadding(FMargin(22.f, 11.f, 22.f, 10.f));
	return Style;
}

UBorder* SokobanStyle::MakePanel(UWidgetTree& Tree, const FMargin& Padding)
{
	UBorder* Panel = Tree.ConstructWidget<UBorder>();
	Panel->SetBrush(PanelBrush());
	Panel->SetPadding(Padding);
	return Panel;
}

UWidget* SokobanStyle::MakeRibbon(UWidgetTree& Tree, const FText& Title, int32 Size, UTextBlock** OutText)
{
	UBorder* Ribbon = Tree.ConstructWidget<UBorder>();
	Ribbon->SetBrush(RibbonBrush());
	// The band sits in the upper part of the image, the tails hang below it.
	Ribbon->SetPadding(FMargin(84.f, 4.f, 84.f, 22.f));
	Ribbon->SetHorizontalAlignment(HAlign_Center);
	Ribbon->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Text = MakeSceneText(Tree, Title, Size, ESokobanFontWeight::Heavy, FLinearColor::White);
	Text->SetJustification(ETextJustify::Center);
	Ribbon->SetContent(Text);

	USizeBox* Box = Tree.ConstructWidget<USizeBox>();
	Box->SetHeightOverride(84.f);
	Box->SetMinDesiredWidth(300.f);
	Box->SetContent(Ribbon);
	if (OutText)
	{
		*OutText = Text;
	}
	return Box;
}

void SokobanStyle::AddKeyHint(UWidgetTree& Tree, UHorizontalBox& Row, const FText& Key, const FText& Action, bool bOnDark)
{
	// A little key, with a lip like the buttons (the card frame drawn small).
	UBorder* Cap = Tree.ConstructWidget<UBorder>();
	Cap->SetBrush(BoxBrush(TEXT("T_UICard"), FVector2D(32.f, 32.f), FMargin(0.3f), Paper));
	Cap->SetPadding(FMargin(13.f, 1.f, 13.f, 6.f));
	Cap->SetContent(MakeText(Tree, Key, 14, ESokobanFontWeight::Bold));
	UHorizontalBoxSlot* CapSlot = Row.AddChildToHorizontalBox(Cap);
	CapSlot->SetVerticalAlignment(VAlign_Center);
	CapSlot->SetPadding(FMargin(Row.GetChildrenCount() > 1 ? 18.f : 0.f, 0.f, 6.f, 0.f));

	UTextBlock* Label = MakeText(Tree, Action, 15, ESokobanFontWeight::Bold, bOnDark ? Paper : InkSoft);
	Row.AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
}

FSlateBrush SokobanStyle::StarBrush(bool bEarned, float Size)
{
	return ImageBrush(UITexture(bEarned ? TEXT("T_StarFull") : TEXT("T_StarEmpty")), Size, FLinearColor::White);
}

FSlateBrush SokobanStyle::GlowBrush(float Size, const FLinearColor& Tint)
{
	return ImageBrush(UITexture(TEXT("T_StarGlow")), Size, Tint);
}
