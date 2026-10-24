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
	UOverlaySlot* RibbonSlot = Frame->AddChildToOverlay(SokobanStyle::MakeRibbon(*WidgetTree, LOCTEXT("Title", "选择关卡"), 34));
	RibbonSlot->SetHorizontalAlignment(HAlign_Center);
	RibbonSlot->SetVerticalAlignment(VAlign_Top);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	PackText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 17, ESokobanFontWeight::Bold, SokobanStyle::InkSoft);
	PackText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* PackSlot = Column->AddChildToVerticalBox(PackText);
	PackSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	PackSlot->SetHorizontalAlignment(HAlign_Fill);

	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(8.f));
	Column->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Center);

	BackButton = WidgetTree->ConstructWidget<USokobanButton>();
	BackButton->SetTone(ESokobanButtonTone::Paper);
	UTextBlock* BackLabel = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Back", "返回"), 20, ESokobanFontWeight::Bold);
	if (UButtonSlot* BackLabelSlot = Cast<UButtonSlot>(BackButton->SetContent(BackLabel)))
	{
		BackLabelSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(BackButton);
	BackSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
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
		Button->SetTone(ESokobanButtonTone::Paper);
		Button->SetIsEnabled(Card.bUnlocked);
		Button->OnActivated = [OnChosen, Index]() { OnChosen(Index); };

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
		CardSize->SetWidthOverride(284.f);
		CardSize->SetHeightOverride(96.f);
		Button->SetContent(CardSize);
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		CardSize->SetContent(Line);

		// A medallion with the level's number (gold once solved, a padlock while locked)...
		USizeBox* Medallion = WidgetTree->ConstructWidget<USizeBox>();
		Medallion->SetWidthOverride(56.f);
		Medallion->SetHeightOverride(56.f);
		UOverlay* MedallionLayers = WidgetTree->ConstructWidget<UOverlay>();
		Medallion->SetContent(MedallionLayers);
		UImage* Disc = WidgetTree->ConstructWidget<UImage>();
		Disc->SetBrush(SokobanStyle::CircleBrush(!Card.bUnlocked ? SokobanStyle::Grey : (Card.bSolved ? SokobanStyle::Gold : SokobanStyle::Sea), 56.f));
		MedallionLayers->AddChildToOverlay(Disc);
		UWidget* Mark = nullptr;
		if (Card.bUnlocked)
		{
			Mark = SokobanStyle::MakeSceneText(*WidgetTree, FText::AsNumber(Index + 1), 26, ESokobanFontWeight::Heavy, FLinearColor::White);
		}
		else
		{
			UImage* Lock = WidgetTree->ConstructWidget<UImage>();
			Lock->SetBrush(SokobanStyle::IconBrush(TEXT("Lock"), 30.f));
			Mark = Lock;
		}
		UOverlaySlot* MarkSlot = MedallionLayers->AddChildToOverlay(Mark);
		MarkSlot->SetHorizontalAlignment(HAlign_Center);
		MarkSlot->SetVerticalAlignment(VAlign_Center);
		MarkSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		Line->AddChildToHorizontalBox(Medallion)->SetVerticalAlignment(VAlign_Center);

		// ...then its name, the stars earned so far (empty slots until solved) and how it went.
		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* LinesSlot = Line->AddChildToHorizontalBox(Lines);
		LinesSlot->SetVerticalAlignment(VAlign_Center);
		LinesSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
		Lines->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Card.Name, 19, ESokobanFontWeight::Bold, Card.bUnlocked ? SokobanStyle::Ink : SokobanStyle::InkSoft));
		UHorizontalBox* StarsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Lines->AddChildToVerticalBox(StarsRow)->SetPadding(FMargin(0.f, 3.f, 0.f, 1.f));
		for (int32 Star = 0; Card.bUnlocked && Star < FSokobanRules::MaxStars; ++Star)
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrush(SokobanStyle::StarBrush(Star < Card.Stars, 22.f));
			StarsRow->AddChildToHorizontalBox(Image)->SetPadding(FMargin(0.f, 0.f, 2.f, 0.f));
		}
		Lines->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, Card.Status, 13, ESokobanFontWeight::Medium, SokobanStyle::InkSoft));

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
