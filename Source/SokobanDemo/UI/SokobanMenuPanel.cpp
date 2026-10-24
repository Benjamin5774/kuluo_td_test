// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanMenuPanel.h"
#include "Puzzle/SokobanRules.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace StarReveal
{
	constexpr int32 SparksPerStar = 6;

	/** Seconds before the first star, and between stars. */
	constexpr float FirstDelay = 0.25f;
	constexpr float Interval = 0.32f;

	/** Seconds a star takes to fall into its slot, then to bounce, glow, throw sparks and jolt the panel. */
	constexpr float SlamTime = 0.2f;
	constexpr float SettleTime = 0.35f;
	constexpr float GlowTime = 0.4f;
	constexpr float SparkTime = 0.5f;
	constexpr float JoltTime = 0.22f;

	/** The middle star is bigger and sits a little higher. */
	float StarSize(int32 Index)
	{
		return Index == 1 ? 112.f : 92.f;
	}
}

void USokobanMenuPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Dim the board behind the panel.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Dim, 0.f));
	UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop);
	BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
	BackdropSlot->SetVerticalAlignment(VAlign_Fill);

	PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Panel, 18.f));
	PanelBorder->SetPadding(FMargin(48.f, 36.f));
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBorder);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetMinDesiredWidth(420.f);
	Width->SetMaxDesiredWidth(560.f);
	PanelBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Width->SetContent(Column);

	TitleText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 40, true, SokobanStyle::Accent);
	TitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Fill);

	// The rating row: an empty slot per star, with the earned look, its glow and its sparks stacked on top,
	// all hidden until ShowStars reveals them.
	StarRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	StarRow->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBoxSlot* StarRowSlot = Column->AddChildToVerticalBox(StarRow);
	StarRowSlot->SetHorizontalAlignment(HAlign_Center);
	StarRowSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 4.f));
	for (int32 Index = 0; Index < FSokobanRules::MaxStars; ++Index)
	{
		const float Size = StarReveal::StarSize(Index);
		USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>();
		Cell->SetWidthOverride(Size);
		Cell->SetHeightOverride(Size);
		Cell->SetRenderTranslation(FVector2D(0.f, Index == 1 ? -10.f : 0.f));
		UHorizontalBoxSlot* CellSlot = StarRow->AddChildToHorizontalBox(Cell);
		CellSlot->SetPadding(FMargin(10.f, 0.f));
		CellSlot->SetVerticalAlignment(VAlign_Center);

		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
		Cell->SetContent(Layers);
		auto AddLayer = [this, Layers](const FSlateBrush& Brush, bool bHidden)
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrush(Brush);
			Image->SetRenderOpacity(bHidden ? 0.f : 1.f);
			UOverlaySlot* LayerSlot = Layers->AddChildToOverlay(Image);
			LayerSlot->SetHorizontalAlignment(HAlign_Center);
			LayerSlot->SetVerticalAlignment(VAlign_Center);
			return Image;
		};
		GlowImages.Add(AddLayer(SokobanStyle::GlowBrush(Size * 2.4f, FLinearColor(1.f, 0.78f, 0.3f)), true));
		AddLayer(SokobanStyle::StarBrush(false, Size), false);
		StarImages.Add(AddLayer(SokobanStyle::StarBrush(true, Size), true));
		for (int32 Spark = 0; Spark < StarReveal::SparksPerStar; ++Spark)
		{
			SparkImages.Add(AddLayer(SokobanStyle::StarBrush(true, 24.f), true));
		}
	}

	MessageText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 18);
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(MessageText);
	MessageSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));
	MessageSlot->SetHorizontalAlignment(HAlign_Fill);

	ButtonColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBoxSlot* ButtonsSlot = Column->AddChildToVerticalBox(ButtonColumn);
	ButtonsSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	ButtonsSlot->SetHorizontalAlignment(HAlign_Fill);
}

void USokobanMenuPanel::Present(const FText& Title, const FText& Message, TArray<FSokobanMenuItem> Items, TFunction<void()> OnBack)
{
	StarRow->SetVisibility(ESlateVisibility::Collapsed);
	StarClock = -1.f;
	PanelBorder->SetRenderTranslation(FVector2D::ZeroVector);

	TitleText->SetText(Title);
	MessageText->SetText(Message);
	MessageText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	BackAction = MoveTemp(OnBack);

	ButtonColumn->ClearChildren();
	Buttons.Reset();
	for (FSokobanMenuItem& Item : Items)
	{
		USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
		Button->SetStyle(SokobanStyle::ButtonStyle());
		Button->SetIsEnabled(Item.bEnabled);
		Button->OnActivated = MoveTemp(Item.OnActivated);

		UTextBlock* Label = SokobanStyle::MakeText(*WidgetTree, Item.Label, 22, true, Item.bEnabled ? SokobanStyle::Text : SokobanStyle::TextDim);
		Label->SetJustification(ETextJustify::Center);
		if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->SetContent(Label)))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UVerticalBoxSlot* ButtonSlot = ButtonColumn->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(0.f, 6.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		Buttons.Add(Button);
	}

	FocusFirstButton();
}

void USokobanMenuPanel::ShowStars(int32 Earned)
{
	StarsEarned = FMath::Clamp(Earned, 0, StarImages.Num());
	StarClock = 0.f;
	StarRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	AnimateStars();
}

void USokobanMenuPanel::AnimateStars()
{
	using namespace StarReveal;

	FVector2D Jolt = FVector2D::ZeroVector;
	for (int32 Index = 0; Index < StarImages.Num(); ++Index)
	{
		UImage* Star = StarImages[Index];
		UImage* Glow = GlowImages[Index];
		const float SinceStart = StarClock - (FirstDelay + Index * Interval);
		const float SinceLanding = SinceStart - SlamTime;

		// A star not earned, or not yet on its way, leaves its slot empty.
		const bool bShown = Index < StarsEarned && SinceStart >= 0.f;
		Star->SetRenderOpacity(bShown ? 1.f : 0.f);
		Glow->SetRenderOpacity(0.f);
		for (int32 Spark = 0; Spark < SparksPerStar; ++Spark)
		{
			SparkImages[Index * SparksPerStar + Spark]->SetRenderOpacity(0.f);
		}
		if (!bShown)
		{
			continue;
		}

		if (SinceLanding < 0.f)
		{
			// Falling at the screen: huge and tilted at first, shrinking ever faster into the slot.
			const float Fall = FMath::Square(SinceStart / SlamTime);
			Star->SetRenderScale(FVector2D(FMath::Lerp(3.2f, 0.8f, Fall)));
			Star->SetRenderTransformAngle(FMath::Lerp(-35.f, 0.f, Fall));
			Star->SetRenderOpacity(FMath::Min(1.f, SinceStart / SlamTime * 2.5f));
			continue;
		}

		// Landed: squashed small by the impact, it springs back through a slight overshoot...
		const float Settle = FMath::Min(SinceLanding / SettleTime, 1.f);
		Star->SetRenderScale(FVector2D(1.f - 0.2f * FMath::Cos(Settle * 2.5f * PI) * FMath::Square(1.f - Settle)));
		Star->SetRenderTransformAngle(0.f);

		// ...flashing a glow behind it...
		const float GlowAge = FMath::Min(SinceLanding / GlowTime, 1.f);
		Glow->SetRenderScale(FVector2D(0.5f + 0.9f * GlowAge));
		Glow->SetRenderOpacity(0.9f * FMath::Square(1.f - GlowAge));

		// ...and throwing out a ring of spinning sparks.
		const float SparkAge = FMath::Min(SinceLanding / SparkTime, 1.f);
		for (int32 Spark = 0; Spark < SparksPerStar; ++Spark)
		{
			UImage* Image = SparkImages[Index * SparksPerStar + Spark];
			const float Angle = FMath::DegreesToRadians((Spark + 0.5f) * 360.f / SparksPerStar + Index * 17.f);
			const float Distance = 95.f * (1.f - FMath::Cube(1.f - SparkAge));
			Image->SetRenderTranslation(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance);
			Image->SetRenderScale(FVector2D(1.f - 0.6f * SparkAge));
			Image->SetRenderTransformAngle(240.f * SparkAge);
			Image->SetRenderOpacity(1.f - SparkAge);
		}

		// The panel jolts with each landing, hardest for the last star.
		if (SinceLanding < JoltTime)
		{
			const float Fade = FMath::Square(1.f - SinceLanding / JoltTime);
			const float Strength = (Index == FSokobanRules::MaxStars - 1 ? 11.f : 6.f) * Fade;
			Jolt = FVector2D(FMath::Sin(SinceLanding * 95.f), FMath::Cos(SinceLanding * 70.f)) * Strength;
		}
	}
	PanelBorder->SetRenderTranslation(Jolt);

	// Stop once the last earned star has settled.
	const float End = FirstDelay + FMath::Max(StarsEarned - 1, 0) * Interval + SlamTime + FMath::Max(SettleTime, SparkTime);
	if (StarClock > End)
	{
		StarClock = -1.f;
	}
}

void USokobanMenuPanel::FocusFirstButton()
{
	for (USokobanButton* Button : Buttons)
	{
		if (Button->GetIsEnabled())
		{
			Button->SetKeyboardFocus();
			return;
		}
	}
	SetKeyboardFocus();
}

void USokobanMenuPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	for (USokobanButton* Button : Buttons)
	{
		Button->UpdateFocusLook();
	}

	if (StarClock >= 0.f)
	{
		StarClock += InDeltaTime;
		AnimateStars();
	}
}

FReply USokobanMenuPanel::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (BackAction && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right))
	{
		// Copy first: the action may re-present this panel and replace BackAction while it runs.
		const TFunction<void()> Action = BackAction;
		Action();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
