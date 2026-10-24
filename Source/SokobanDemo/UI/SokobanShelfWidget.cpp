// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanShelfWidget.h"
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
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "SokobanShelf"

void USokobanShelfWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UImage* Backdrop = WidgetTree->ConstructWidget<UImage>();
	Backdrop->SetBrush(SokobanStyle::VignetteBrush());
	UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop);
	BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
	BackdropSlot->SetVerticalAlignment(VAlign_Fill);

	// The paper panel, its title on a ribbon across the top edge.
	UOverlay* Frame = WidgetTree->ConstructWidget<UOverlay>();
	UOverlaySlot* FrameSlot = Root->AddChildToOverlay(Frame);
	FrameSlot->SetHorizontalAlignment(HAlign_Center);
	FrameSlot->SetVerticalAlignment(VAlign_Center);
	UBorder* Panel = SokobanStyle::MakePanel(*WidgetTree, FMargin(40.f, 58.f, 40.f, 38.f));
	Frame->AddChildToOverlay(Panel)->SetPadding(FMargin(0.f, 40.f, 0.f, 0.f));
	UTextBlock* Title = nullptr;
	UOverlaySlot* RibbonSlot = Frame->AddChildToOverlay(SokobanStyle::MakeRibbon(*WidgetTree, FText::GetEmpty(), 34, &Title));
	RibbonSlot->SetHorizontalAlignment(HAlign_Center);
	RibbonSlot->SetVerticalAlignment(VAlign_Top);
	TitleText = Title;

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetWidthOverride(680.f);
	Panel->SetContent(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Width->SetContent(Column);

	EmptyMessage = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 18, ESokobanFontWeight::Bold, SokobanStyle::InkSoft);
	EmptyMessage->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(EmptyMessage)->SetPadding(FMargin(0.f, 16.f));

	USizeBox* ListHeight = WidgetTree->ConstructWidget<USizeBox>();
	ListHeight->SetMaxDesiredHeight(420.f);
	List = WidgetTree->ConstructWidget<UScrollBox>();
	ListHeight->SetContent(List);
	Column->AddChildToVerticalBox(ListHeight);

	BackButton = WidgetTree->ConstructWidget<USokobanButton>();
	BackButton->SetTone(ESokobanButtonTone::Paper);
	BackButton->OnActivated = [this]()
	{
		// Copy first: the action may re-present this list and replace BackAction while it runs.
		if (const TFunction<void()> Action = BackAction)
		{
			Action();
		}
	};
	UTextBlock* BackLabel = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Back", "返回"), 20, ESokobanFontWeight::Bold);
	if (UButtonSlot* BackLabelSlot = Cast<UButtonSlot>(BackButton->SetContent(BackLabel)))
	{
		BackLabelSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(BackButton);
	BackSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
	BackSlot->SetHorizontalAlignment(HAlign_Center);
}

void USokobanShelfWidget::Present(const FText& Title, const FText& EmptyText, TArray<FSokobanShelfRow> Rows, TFunction<void()> OnBack)
{
	TitleText->SetText(Title);
	EmptyMessage->SetText(EmptyText);
	EmptyMessage->SetVisibility(Rows.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BackAction = MoveTemp(OnBack);

	List->ClearChildren();
	Buttons.Reset();
	for (FSokobanShelfRow& Row : Rows)
	{
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrush(SokobanStyle::CardBrush(FLinearColor::White));
		Card->SetPadding(FMargin(20.f, 10.f, 14.f, 14.f));
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(List->AddChild(Card)))
		{
			CardSlot->SetPadding(FMargin(0.f, 5.f));
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Card->SetContent(Line);

		UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
		Texts->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Row.Name, 20, ESokobanFontWeight::Bold));
		Texts->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Row.Info, 14, ESokobanFontWeight::Medium, SokobanStyle::InkSoft))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);

		for (FSokobanMenuItem& Action : Row.Actions)
		{
			USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
			Button->SetTone(Action.Tone);
			Button->SetIsEnabled(Action.bEnabled);
			Button->OnActivated = MoveTemp(Action.OnActivated);
			Button->SetContent(SokobanStyle::MakeText(*WidgetTree, Action.Label, 16, ESokobanFontWeight::Bold, Action.bEnabled ? SokobanStyle::Ink : SokobanStyle::InkSoft));
			UHorizontalBoxSlot* ButtonSlot = Line->AddChildToHorizontalBox(Button);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
			ButtonSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
			Buttons.Add(Button);
		}
	}

	FocusFirstButton();
}

void USokobanShelfWidget::FocusFirstButton()
{
	for (USokobanButton* Button : Buttons)
	{
		if (Button->GetIsEnabled())
		{
			Button->SetKeyboardFocus();
			return;
		}
	}
	BackButton->SetKeyboardFocus();
}

bool USokobanShelfWidget::ActivateButton(int32 Index)
{
	if (!Buttons.IsValidIndex(Index) || !Buttons[Index]->GetIsEnabled() || !Buttons[Index]->OnActivated)
	{
		return false;
	}
	const TFunction<void()> Action = Buttons[Index]->OnActivated;
	Action();
	return true;
}

void USokobanShelfWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	for (USokobanButton* Button : Buttons)
	{
		Button->UpdateFocusLook();
	}
	BackButton->UpdateFocusLook();
}

FReply USokobanShelfWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (BackAction && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		// Copy first, as for the back button.
		const TFunction<void()> Action = BackAction;
		Action();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
