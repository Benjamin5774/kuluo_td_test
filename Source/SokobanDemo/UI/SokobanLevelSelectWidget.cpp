// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanLevelSelectWidget.h"
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
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "SokobanLevelSelect"

void USokobanLevelSelectWidget::NativeOnInitialized()
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

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	UTextBlock* Title = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Title", "选择关卡"), 36, true, SokobanStyle::Accent);
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Fill);

	PackText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 16, false, SokobanStyle::TextDim);
	PackText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* PackSlot = Column->AddChildToVerticalBox(PackText);
	PackSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 18.f));
	PackSlot->SetHorizontalAlignment(HAlign_Fill);

	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(8.f));
	Column->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Center);

	BackButton = WidgetTree->ConstructWidget<USokobanButton>();
	BackButton->SetStyle(SokobanStyle::ButtonStyle());
	UTextBlock* BackLabel = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Back", "返回"), 20, true);
	if (UButtonSlot* BackLabelSlot = Cast<UButtonSlot>(BackButton->SetContent(BackLabel)))
	{
		BackLabelSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(BackButton);
	BackSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
	BackSlot->SetHorizontalAlignment(HAlign_Center);
}

void USokobanLevelSelectWidget::Present(const FText& PackName, const TArray<FSokobanLevelCard>& Cards, int32 FocusIndex, TFunction<void(int32)> OnChosen, TFunction<void()> OnBack)
{
	PackText->SetText(PackName);
	BackAction = OnBack;
	BackButton->OnActivated = MoveTemp(OnBack);
	PreferredFocus = FocusIndex;

	Grid->ClearChildren();
	CardButtons.Reset();
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FSokobanLevelCard& Card = Cards[Index];

		USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
		Button->SetStyle(SokobanStyle::ButtonStyle());
		Button->SetIsEnabled(Card.bUnlocked);
		Button->OnActivated = [OnChosen, Index]() { OnChosen(Index); };

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
		CardSize->SetWidthOverride(190.f);
		CardSize->SetHeightOverride(104.f);
		Button->SetContent(CardSize);

		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>();
		CardSize->SetContent(Lines);

		// The level number, with the stars earned so far (empty slots until solved) at the right.
		UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Lines->AddChildToVerticalBox(TopRow)->SetHorizontalAlignment(HAlign_Fill);
		const FLinearColor NumberColor = !Card.bUnlocked ? SokobanStyle::TextDim : (Card.bSolved ? SokobanStyle::Accent : SokobanStyle::Text);
		UTextBlock* Number = SokobanStyle::MakeText(*WidgetTree, FText::AsNumber(Index + 1), 30, true, NumberColor);
		UHorizontalBoxSlot* NumberSlot = TopRow->AddChildToHorizontalBox(Number);
		NumberSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		if (Card.bUnlocked)
		{
			for (int32 Star = 0; Star < FSokobanRules::MaxStars; ++Star)
			{
				UImage* Image = WidgetTree->ConstructWidget<UImage>();
				Image->SetBrush(SokobanStyle::StarBrush(Star < Card.Stars, 24.f));
				UHorizontalBoxSlot* StarSlot = TopRow->AddChildToHorizontalBox(Image);
				StarSlot->SetVerticalAlignment(VAlign_Center);
				StarSlot->SetPadding(FMargin(1.f, 0.f));
			}
		}

		UTextBlock* Name = SokobanStyle::MakeText(*WidgetTree, Card.Name, 17, true, Card.bUnlocked ? SokobanStyle::Text : SokobanStyle::TextDim);
		Lines->AddChildToVerticalBox(Name);

		UTextBlock* Status = SokobanStyle::MakeText(*WidgetTree, Card.Status, 13, false, Card.bSolved ? SokobanStyle::Accent : SokobanStyle::TextDim);
		Lines->AddChildToVerticalBox(Status)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

		Grid->AddChildToUniformGrid(Button, Index / Columns, Index % Columns);
		CardButtons.Add(Button);
	}

	FocusPreferredCard();
}

void USokobanLevelSelectWidget::FocusPreferredCard()
{
	if (CardButtons.IsValidIndex(PreferredFocus) && CardButtons[PreferredFocus]->GetIsEnabled())
	{
		CardButtons[PreferredFocus]->SetKeyboardFocus();
	}
	else
	{
		BackButton->SetKeyboardFocus();
	}
}

void USokobanLevelSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	for (USokobanButton* Button : CardButtons)
	{
		Button->UpdateFocusLook();
	}
	BackButton->UpdateFocusLook();
}

FReply USokobanLevelSelectWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (BackAction && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		const TFunction<void()> Action = BackAction;
		Action();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
