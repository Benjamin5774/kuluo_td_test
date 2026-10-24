// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanTitleWidget.h"
#include "Game/SokobanAudioSubsystem.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "SokobanTitle"

namespace
{
	/** Seconds a letter takes to fall in, and between one letter starting to fall and the next. */
	constexpr float FallTime = 0.65f;
	constexpr float LetterStagger = 0.12f;

	/** 0 to 1 with a ball's bounces at the end (the usual "bounce out" easing). */
	float BounceOut(float T)
	{
		constexpr float N = 7.5625f;
		constexpr float D = 2.75f;
		if (T < 1.f / D)
		{
			return N * T * T;
		}
		if (T < 2.f / D)
		{
			T -= 1.5f / D;
			return N * T * T + 0.75f;
		}
		if (T < 2.5f / D)
		{
			T -= 2.25f / D;
			return N * T * T + 0.9375f;
		}
		T -= 2.625f / D;
		return N * T * T + 0.984375f;
	}

	float Ease(float From, float To, float Clock)
	{
		const float T = FMath::Clamp((Clock - From) / (To - From), 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - T);
	}
}

void USokobanTitleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// The name, top left over the sky: the island in sea blue, the adventure in sunny gold, thick dark outlines.
	UVerticalBox* TitleBlock = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(TitleBlock);
	TitleSlot->SetAnchors(FAnchors(0.f, 0.f));
	TitleSlot->SetPosition(FVector2D(92.f, 58.f));
	TitleSlot->SetAutoSize(true);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	TitleBlock->AddChildToVerticalBox(Row);
	const FString Name = LOCTEXT("GameName", "海岛大冒险").ToString();
	for (int32 Index = 0; Index < Name.Len(); ++Index)
	{
		const FLinearColor Color = Index < 2 ? FLinearColor(0.45f, 0.9f, 1.f) : FLinearColor(1.f, 0.8f, 0.24f);
		UTextBlock* Letter = SokobanStyle::MakeText(*WidgetTree, FText::FromString(Name.Mid(Index, 1)), 112, ESokobanFontWeight::Heavy, Color);
		Letter->SetFont(SokobanStyle::OutlinedFont(112, ESokobanFontWeight::Heavy, 9));
		Letter->SetShadowOffset(FVector2D(5.f, 8.f));
		Letter->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.4f));
		Letter->SetRenderTransformPivot(FVector2D(0.5f, 0.7f));
		Row->AddChildToHorizontalBox(Letter)->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
		Letters.Add(Letter);
	}

	Subtitle = SokobanStyle::MakeSceneText(*WidgetTree, LOCTEXT("Subtitle", "胖橘的推箱子冒险"), 32, ESokobanFontWeight::Bold);
	TitleBlock->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(14.f, 2.f, 0.f, 0.f));

	// The menu, bottom left.
	ButtonColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* MenuSlot = Root->AddChildToCanvas(ButtonColumn);
	MenuSlot->SetAnchors(FAnchors(0.f, 1.f));
	MenuSlot->SetAlignment(FVector2D(0.f, 1.f));
	MenuSlot->SetPosition(FVector2D(72.f, -76.f));
	MenuSlot->SetAutoSize(true);

	// Only the buttons take clicks.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TitleBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USokobanTitleWidget::Present(TArray<FSokobanMenuItem> Items, bool bIntro)
{
	ButtonColumn->ClearChildren();
	Buttons.Reset();
	Rows.Reset();
	Paws.Reset();
	for (FSokobanMenuItem& Item : Items)
	{
		USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
		Button->SetTone(Item.Tone);
		Button->SetIsEnabled(Item.bEnabled);
		Button->OnActivated = MoveTemp(Item.OnActivated);

		UTextBlock* Label = SokobanStyle::MakeText(*WidgetTree, Item.Label, 24, ESokobanFontWeight::Bold, Item.bEnabled ? SokobanStyle::Ink : SokobanStyle::InkSoft);
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetMinDesiredWidth(236.f);
		Width->SetContent(Label);
		Button->SetContent(Width);

		// A paw beside the button points at the choice.
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UImage* Paw = WidgetTree->ConstructWidget<UImage>();
		Paw->SetBrush(SokobanStyle::IconBrush(TEXT("Paw"), 40.f));
		Paw->SetRenderOpacity(0.f);
		Paw->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UHorizontalBoxSlot* PawSlot = Row->AddChildToHorizontalBox(Paw);
		PawSlot->SetVerticalAlignment(VAlign_Center);
		PawSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		Row->AddChildToHorizontalBox(Button)->SetVerticalAlignment(VAlign_Center);

		ButtonColumn->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 5.f));
		Buttons.Add(Button);
		Rows.Add(Row);
		Paws.Add(Paw);
	}

	// Without the intro, start late enough that everything has arrived (the letters keep bobbing either way).
	Clock = bIntro ? 0.f : 60.f;
	LettersLanded = bIntro ? 0 : Letters.Num();
	Animate();
	FocusFirstButton();
}

void USokobanTitleWidget::FocusFirstButton()
{
	for (USokobanButton* Button : Buttons)
	{
		if (Button->GetIsEnabled())
		{
			Button->SetKeyboardFocus();
			return;
		}
	}
}

bool USokobanTitleWidget::ActivateButton(int32 Index)
{
	if (!Buttons.IsValidIndex(Index) || !Buttons[Index]->GetIsEnabled() || !Buttons[Index]->OnActivated)
	{
		return false;
	}
	// Copy first: the action may re-present this screen.
	const TFunction<void()> Action = Buttons[Index]->OnActivated;
	Action();
	return true;
}

void USokobanTitleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Clock += InDeltaTime;
	Animate();
	for (USokobanButton* Button : Buttons)
	{
		Button->UpdateFocusLook();
	}
}

void USokobanTitleWidget::Animate()
{
	for (int32 Index = 0; Index < Letters.Num(); ++Index)
	{
		// Each letter drops in from above, bouncing to rest, then rocks gently like a boat, out of step with the next.
		const float Start = 0.25f + LetterStagger * Index;
		const float Drop = FMath::Clamp((Clock - Start) / FallTime, 0.f, 1.f);
		// A wooden knock as each letter first hits the ground, a little higher each time.
		if (Index >= LettersLanded && Drop >= 1.f / 2.75f)
		{
			LettersLanded = Index + 1;
			if (USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this))
			{
				Audio->Play(ESokobanSound::Click, 0.45f, 0.62f + 0.07f * Index);
			}
		}
		const float Afloat = Ease(Start + FallTime, Start + FallTime + 0.8f, Clock);
		const float Bob = Afloat * 7.f * FMath::Sin(Clock * 2.f + Index * 0.9f);
		FWidgetTransform Pose;
		Pose.Translation = FVector2D(0.f, -340.f * (1.f - BounceOut(Drop)) + Bob);
		Pose.Angle = -16.f * FMath::Square(1.f - Drop) + Afloat * 2.5f * FMath::Sin(Clock * 1.5f + Index * 1.4f);
		Letters[Index]->SetRenderTransform(Pose);
		Letters[Index]->SetRenderOpacity(FMath::Clamp(Drop * 6.f, 0.f, 1.f));
	}

	const float TitleLanded = 0.25f + LetterStagger * (Letters.Num() - 1) + FallTime;
	const float SubtitleIn = Ease(TitleLanded - 0.1f, TitleLanded + 0.5f, Clock);
	Subtitle->SetRenderOpacity(SubtitleIn);
	Subtitle->SetRenderTranslation(FVector2D(0.f, 16.f * (1.f - SubtitleIn)));

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const float Start = TitleLanded + 0.15f + 0.08f * Index;
		const float In = Ease(Start, Start + 0.45f, Clock);
		Rows[Index]->SetRenderTranslation(FVector2D(-110.f * (1.f - In), 0.f));
		Rows[Index]->SetRenderOpacity(In);

		// The paw pats the air beside the chosen button.
		const bool bChosen = Buttons[Index]->HasKeyboardFocus() || Buttons[Index]->IsHovered();
		FWidgetTransform PawPose;
		PawPose.Translation = FVector2D(-6.f * FMath::Abs(FMath::Sin(Clock * 4.f)), 0.f);
		PawPose.Angle = 90.f;
		Paws[Index]->SetRenderTransform(PawPose);
		Paws[Index]->SetRenderOpacity(bChosen ? 1.f : 0.f);
	}
}

#undef LOCTEXT_NAMESPACE
