// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanHUDWidget.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "SokobanHUD"

void USokobanHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Header = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 28, true);
	StatsText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 18, false, SokobanStyle::Accent);
	Header->AddChildToVerticalBox(TitleText);
	Header->AddChildToVerticalBox(StatsText);

	UCanvasPanelSlot* HeaderSlot = Root->AddChildToCanvas(Header);
	HeaderSlot->SetAnchors(FAnchors(0.f, 0.f));
	HeaderSlot->SetPosition(FVector2D(36.f, 28.f));
	HeaderSlot->SetAutoSize(true);

	UTextBlock* Hint = SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Hint", "WASD / 方向键  移动      Z  撤销（按住快退）      R  重开      Esc  暂停"), 15, false, SokobanStyle::Text);
	UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(Hint);
	HintSlot->SetAnchors(FAnchors(0.5f, 1.f));
	HintSlot->SetAlignment(FVector2D(0.5f, 1.f));
	HintSlot->SetPosition(FVector2D(0.f, -22.f));
	HintSlot->SetAutoSize(true);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USokobanHUDWidget::SetLevelTitle(const FText& Title)
{
	TitleText->SetText(Title);
}

void USokobanHUDWidget::SetStats(const FText& Stats)
{
	StatsText->SetText(Stats);
}

#undef LOCTEXT_NAMESPACE
