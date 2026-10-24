// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanShelfWidget.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Dim, 0.f));
	UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop);
	BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
	BackdropSlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Panel, 18.f));
	Panel->SetPadding(FMargin(40.f, 32.f));
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetWidthOverride(660.f);
	Panel->SetContent(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Width->SetContent(Column);

	TitleText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 36, true, SokobanStyle::Accent);
	TitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Fill);

	EmptyMessage = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 18, false, SokobanStyle::TextDim);
	EmptyMessage->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(EmptyMessage)->SetPadding(FMargin(0.f, 24.f));

	USizeBox* ListHeight = WidgetTree->ConstructWidget<USizeBox>();
	ListHeight->SetMaxDesiredHeight(430.f);
	List = WidgetTree->ConstructWidget<UScrollBox>();
	ListHeight->SetContent(List);
	Column->AddChildToVerticalBox(ListHeight)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	BackButton = WidgetTree->ConstructWidget<USokobanButton>();
	BackButton->SetStyle(SokobanStyle::ButtonStyle());
	BackButton->OnActivated = [this]()
	{
		// Copy first: the action may re-present this list and replace BackAction while it runs.
		if (const TFunction<void()> Action = BackAction)
		{
			Action();
		}
	};
	UTextBlock* BackLabel = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Back", "返回"), 20, true);
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
		Card->SetBrush(SokobanStyle::RoundedBrush(FLinearColor(0.09f, 0.1f, 0.13f, 1.f), 10.f));
		Card->SetPadding(FMargin(18.f, 12.f));
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(List->AddChild(Card)))
		{
			CardSlot->SetPadding(FMargin(0.f, 5.f));
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Card->SetContent(Line);

		UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
		Texts->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Row.Name, 20, true));
		Texts->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Row.Info, 14, false, SokobanStyle::TextDim))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);

		for (FSokobanMenuItem& Action : Row.Actions)
		{
			USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
			Button->SetStyle(SokobanStyle::ButtonStyle());
			Button->SetIsEnabled(Action.bEnabled);
			Button->OnActivated = MoveTemp(Action.OnActivated);
			Button->SetContent(SokobanStyle::MakeText(*WidgetTree, Action.Label, 16, true));
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
