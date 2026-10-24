// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanHUDWidget.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "SokobanHUD"

void USokobanHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Header = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* HeaderSlot = Root->AddChildToCanvas(Header);
	HeaderSlot->SetAnchors(FAnchors(0.f, 0.f));
	HeaderSlot->SetPosition(FVector2D(28.f, 22.f));
	HeaderSlot->SetAutoSize(true);

	// The badge: a sea-blue medallion with the level's number, overlapping the paper plate with its name.
	UOverlay* Badge = WidgetTree->ConstructWidget<UOverlay>();
	Header->AddChildToVerticalBox(Badge);
	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>();
	Plate->SetBrush(SokobanStyle::PillBrush(SokobanStyle::Paper));
	Plate->SetPadding(FMargin(52.f, 6.f, 24.f, 9.f));
	NameText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 24, ESokobanFontWeight::Bold);
	Plate->SetContent(NameText);
	UOverlaySlot* PlateSlot = Badge->AddChildToOverlay(Plate);
	PlateSlot->SetVerticalAlignment(VAlign_Center);
	PlateSlot->SetPadding(FMargin(22.f, 0.f, 0.f, 0.f));

	USizeBox* Medallion = WidgetTree->ConstructWidget<USizeBox>();
	Medallion->SetWidthOverride(62.f);
	Medallion->SetHeightOverride(62.f);
	UOverlay* MedallionLayers = WidgetTree->ConstructWidget<UOverlay>();
	Medallion->SetContent(MedallionLayers);
	UImage* Disc = WidgetTree->ConstructWidget<UImage>();
	Disc->SetBrush(SokobanStyle::CircleBrush(SokobanStyle::Sea, 62.f));
	MedallionLayers->AddChildToOverlay(Disc);
	BadgeText = SokobanStyle::MakeSceneText(*WidgetTree, FText::GetEmpty(), 26, ESokobanFontWeight::Heavy, FLinearColor::White);
	UOverlaySlot* BadgeTextSlot = MedallionLayers->AddChildToOverlay(BadgeText);
	BadgeTextSlot->SetHorizontalAlignment(HAlign_Center);
	BadgeTextSlot->SetVerticalAlignment(VAlign_Center);
	BadgeTextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
	UOverlaySlot* MedallionSlot = Badge->AddChildToOverlay(Medallion);
	MedallionSlot->SetHorizontalAlignment(HAlign_Left);
	MedallionSlot->SetVerticalAlignment(VAlign_Center);

	// Under it, chips for the moves made, the par and the best so far.
	UHorizontalBox* Chips = WidgetTree->ConstructWidget<UHorizontalBox>();
	Header->AddChildToVerticalBox(Chips)->SetPadding(FMargin(18.f, 8.f, 0.f, 0.f));
	UTextBlock* Moves = nullptr;
	UTextBlock* Par = nullptr;
	UTextBlock* Best = nullptr;
	AddChip(Chips, SokobanStyle::IconBrush(TEXT("Paw"), 24.f), Moves);
	ParChip = AddChip(Chips, SokobanStyle::StarBrush(true, 26.f), Par);
	BestChip = AddChip(Chips, SokobanStyle::IconBrush(TEXT("Trophy"), 24.f), Best);
	MovesText = Moves;
	ParText = Par;
	BestText = Best;

	// Bottom: the controls, as key caps on a dark bar.
	UBorder* HintBar = WidgetTree->ConstructWidget<UBorder>();
	HintBar->SetBrush(SokobanStyle::RoundedBrush(FLinearColor(SokobanStyle::Ink.R, SokobanStyle::Ink.G, SokobanStyle::Ink.B, 0.62f), 18.f));
	HintBar->SetPadding(FMargin(18.f, 7.f, 18.f, 8.f));
	UHorizontalBox* Hints = WidgetTree->ConstructWidget<UHorizontalBox>();
	HintBar->SetContent(Hints);
	SokobanStyle::AddKeyHint(*WidgetTree, *Hints, LOCTEXT("KeyMove", "WASD / 方向键"), LOCTEXT("Move", "移动"), true);
	SokobanStyle::AddKeyHint(*WidgetTree, *Hints, LOCTEXT("KeyUndo", "Z"), LOCTEXT("Undo", "撤销"), true);
	SokobanStyle::AddKeyHint(*WidgetTree, *Hints, LOCTEXT("KeyRestart", "R"), LOCTEXT("Restart", "重开"), true);
	SokobanStyle::AddKeyHint(*WidgetTree, *Hints, LOCTEXT("KeyZoom", "滚轮"), LOCTEXT("Zoom", "缩放"), true);
	SokobanStyle::AddKeyHint(*WidgetTree, *Hints, LOCTEXT("KeyPause", "Esc"), LOCTEXT("PauseHint", "暂停"), true);
	UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(HintBar);
	HintSlot->SetAnchors(FAnchors(0.5f, 1.f));
	HintSlot->SetAlignment(FVector2D(0.5f, 1.f));
	HintSlot->SetPosition(FVector2D(0.f, -18.f));
	HintSlot->SetAutoSize(true);

	// Pause by mouse too. It never takes keyboard focus, which stays with the game.
	PauseButton = WidgetTree->ConstructWidget<USokobanButton>();
	PauseButton->InitFocusable(false);
	PauseButton->SetTone(ESokobanButtonTone::Paper);
	PauseButton->OnActivated = [this]() { if (OnPause) { OnPause(); } };
	UHorizontalBox* PauseContent = WidgetTree->ConstructWidget<UHorizontalBox>();
	UImage* PauseIcon = WidgetTree->ConstructWidget<UImage>();
	PauseIcon->SetBrush(SokobanStyle::IconBrush(TEXT("Pause"), 22.f));
	PauseContent->AddChildToHorizontalBox(PauseIcon)->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* PauseLabelSlot = PauseContent->AddChildToHorizontalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Pause", "暂停"), 20, ESokobanFontWeight::Bold));
	PauseLabelSlot->SetVerticalAlignment(VAlign_Center);
	PauseLabelSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
	PauseButton->SetContent(PauseContent);
	UCanvasPanelSlot* PauseSlot = Root->AddChildToCanvas(PauseButton);
	PauseSlot->SetAnchors(FAnchors(1.f, 0.f));
	PauseSlot->SetAlignment(FVector2D(1.f, 0.f));
	PauseSlot->SetPosition(FVector2D(-28.f, 24.f));
	PauseSlot->SetAutoSize(true);

	// Only the button takes clicks; the rest lets them through to the game.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Header->SetVisibility(ESlateVisibility::HitTestInvisible);
	HintBar->SetVisibility(ESlateVisibility::HitTestInvisible);
}

UBorder* USokobanHUDWidget::AddChip(UHorizontalBox* Row, const FSlateBrush& Icon, UTextBlock*& OutText)
{
	UBorder* Chip = WidgetTree->ConstructWidget<UBorder>();
	Chip->SetBrush(SokobanStyle::PillBrush(SokobanStyle::Paper));
	Chip->SetPadding(FMargin(10.f, 3.f, 16.f, 6.f));
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
	Chip->SetContent(Content);
	UImage* Image = WidgetTree->ConstructWidget<UImage>();
	Image->SetBrush(Icon);
	Content->AddChildToHorizontalBox(Image)->SetVerticalAlignment(VAlign_Center);
	OutText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 18, ESokobanFontWeight::Bold);
	UHorizontalBoxSlot* TextSlot = Content->AddChildToHorizontalBox(OutText);
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
	Row->AddChildToHorizontalBox(Chip)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	return Chip;
}

void USokobanHUDWidget::SetLevel(const FText& Badge, const FText& Name)
{
	BadgeText->SetText(Badge);
	// A word ("试玩") needs a smaller size than a number to fit the medallion.
	BadgeText->SetFont(SokobanStyle::OutlinedFont(Badge.IsNumeric() ? 26 : 17, ESokobanFontWeight::Heavy, 2));
	NameText->SetText(Name);
}

void USokobanHUDWidget::SetStats(int32 Moves, int32 Par, int32 Best)
{
	MovesText->SetText(FText::Format(LOCTEXT("Moves", "步数 {0}"), Moves));
	ParText->SetText(FText::Format(LOCTEXT("Par", "满星 {0}"), Par));
	BestText->SetText(FText::Format(LOCTEXT("Best", "最佳 {0}"), Best));
	ParChip->SetVisibility(Par > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BestChip->SetVisibility(Best > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
