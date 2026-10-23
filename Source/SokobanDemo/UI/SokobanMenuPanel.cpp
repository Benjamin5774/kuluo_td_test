// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanMenuPanel.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Panel, 18.f));
	Panel->SetPadding(FMargin(48.f, 36.f));
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetMinDesiredWidth(420.f);
	Width->SetMaxDesiredWidth(560.f);
	Panel->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Width->SetContent(Column);

	TitleText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 40, true, SokobanStyle::Accent);
	TitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Fill);

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
