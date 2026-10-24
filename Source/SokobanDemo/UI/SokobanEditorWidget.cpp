// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SokobanEditorWidget.h"
#include "LevelEditor/SokobanLevelEditor.h"
#include "UI/SokobanButton.h"
#include "UI/SokobanUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "SokobanEditorWidget"

namespace
{
	void Place(UCanvasPanel* Root, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(Anchors);
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetPosition(Position);
		PanelSlot->SetAutoSize(true);
	}
}

void USokobanEditorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top left: what is being made.
	UBorder* InfoPanel = SokobanStyle::MakePanel(*WidgetTree, FMargin(26.f, 20.f, 26.f, 26.f));
	Place(Root, InfoPanel, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(20.f, 18.f));
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>();
	InfoPanel->SetContent(Info);
	Info->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Title", "关卡编辑器"), 26, ESokobanFontWeight::Heavy));

	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Info->AddChildToVerticalBox(NameRow)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	UHorizontalBoxSlot* NameLabelSlot = NameRow->AddChildToHorizontalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Name", "名称"), 16, ESokobanFontWeight::Bold, SokobanStyle::InkSoft));
	NameLabelSlot->SetVerticalAlignment(VAlign_Center);
	NameLabelSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	NameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	FEditableTextBoxStyle NameStyle;
	NameStyle.SetFont(SokobanStyle::Font(18, ESokobanFontWeight::Bold))
		.SetForegroundColor(FSlateColor(SokobanStyle::Ink))
		.SetFocusedForegroundColor(FSlateColor(SokobanStyle::Ink))
		.SetBackgroundImageNormal(SokobanStyle::PillBrush(FLinearColor::White))
		.SetBackgroundImageHovered(SokobanStyle::PillBrush(FLinearColor::White))
		.SetBackgroundImageFocused(SokobanStyle::PillBrush(SokobanStyle::GoldLight))
		.SetPadding(FMargin(14.f, 5.f, 14.f, 8.f));
	NameBox->WidgetStyle = NameStyle;
	NameBox->OnTextCommitted.AddDynamic(this, &USokobanEditorWidget::HandleNameCommitted);
	USizeBox* NameWidth = WidgetTree->ConstructWidget<USizeBox>();
	NameWidth->SetWidthOverride(250.f);
	NameWidth->SetContent(NameBox);
	NameRow->AddChildToHorizontalBox(NameWidth);

	UHorizontalBox* SizeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Info->AddChildToVerticalBox(SizeRow)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	SizeText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 16, ESokobanFontWeight::Bold, SokobanStyle::InkSoft);
	USizeBox* SizeWidth = WidgetTree->ConstructWidget<USizeBox>();
	SizeWidth->SetWidthOverride(110.f);
	SizeWidth->SetContent(SizeText);
	SizeRow->AddChildToHorizontalBox(SizeWidth)->SetVerticalAlignment(VAlign_Center);
	const TPair<FText, FIntPoint> SizeSteps[] = {
		{ LOCTEXT("Narrower", "宽 −"), FIntPoint(-1, 0) }, { LOCTEXT("Wider", "宽 ＋"), FIntPoint(1, 0) },
		{ LOCTEXT("Shorter", "高 −"), FIntPoint(0, -1) }, { LOCTEXT("Taller", "高 ＋"), FIntPoint(0, 1) } };
	for (const TPair<FText, FIntPoint>& Step : SizeSteps)
	{
		const FIntPoint Delta = Step.Value;
		USokobanButton* Button = AddToolButton(SizeRow, Step.Key, 0.f, [this, Delta]() { if (Editor.IsValid()) { Editor->Resize(Delta); } });
		Cast<UHorizontalBoxSlot>(Button->Slot)->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
	}

	// Left: the brushes, in ESokobanBrush order, with their number keys and pictures of what they paint.
	UBorder* BrushPanel = SokobanStyle::MakePanel(*WidgetTree, FMargin(18.f, 20.f, 18.f, 24.f));
	Place(Root, BrushPanel, FAnchors(0.f, 0.5f), FVector2D(0.f, 0.5f), FVector2D(20.f, 64.f));
	UVerticalBox* Brushes = WidgetTree->ConstructWidget<UVerticalBox>();
	BrushPanel->SetContent(Brushes);
	Brushes->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Brushes", "笔刷"), 16, ESokobanFontWeight::Bold, SokobanStyle::InkSoft))->SetPadding(FMargin(10.f, 0.f, 0.f, 4.f));
	const TPair<FText, const TCHAR*> BrushLabels[] = {
		{ LOCTEXT("Floor", "1  地面"), TEXT("Floor") }, { LOCTEXT("Wall", "2  墙"), TEXT("Wall") }, { LOCTEXT("Crate", "3  箱子"), TEXT("Crate") },
		{ LOCTEXT("Goal", "4  目标点"), TEXT("Goal") }, { LOCTEXT("Hero", "5  大橘"), TEXT("Hero") }, { LOCTEXT("Plate", "6  机关"), TEXT("Plate") },
		{ LOCTEXT("Barrier", "7  机关方块"), TEXT("Barrier") }, { LOCTEXT("Erase", "8  擦除"), TEXT("Erase") } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BrushLabels); ++Index)
	{
		const ESokobanBrush Brush = static_cast<ESokobanBrush>(Index);
		BrushButtons.Add(AddToolButton(Brushes, BrushLabels[Index].Key, 176.f, [this, Brush]() { if (Editor.IsValid()) { Editor->SetBrush(Brush); } }, BrushLabels[Index].Value));
	}

	// Right: actions; trying and keeping the level stand out.
	UBorder* ActionPanel = SokobanStyle::MakePanel(*WidgetTree, FMargin(18.f, 20.f, 18.f, 24.f));
	Place(Root, ActionPanel, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-20.f, 44.f));
	UVerticalBox* Actions = WidgetTree->ConstructWidget<UVerticalBox>();
	ActionPanel->SetContent(Actions);
	UndoButton = AddToolButton(Actions, LOCTEXT("Undo", "撤销  Ctrl+Z"), 176.f, [this]() { if (Editor.IsValid()) { Editor->Undo(); } });
	RedoButton = AddToolButton(Actions, LOCTEXT("Redo", "重做  Ctrl+Y"), 176.f, [this]() { if (Editor.IsValid()) { Editor->Redo(); } });
	AddToolButton(Actions, LOCTEXT("WallIn", "自动围墙"), 176.f, [this]() { if (Editor.IsValid()) { Editor->WallIn(); } });
	AddToolButton(Actions, LOCTEXT("Check", "检查可解"), 176.f, [this]() { if (Editor.IsValid()) { Editor->CheckSolvable(); } });
	TestButton = AddToolButton(Actions, LOCTEXT("Test", "试玩"), 176.f, [this]() { if (OnTestPlay) { OnTestPlay(); } }, nullptr, ESokobanButtonTone::Sea);
	AddToolButton(Actions, LOCTEXT("Save", "保存关卡"), 176.f, [this]() { if (Editor.IsValid()) { Editor->SaveLevel(); } }, nullptr, ESokobanButtonTone::Sea);
	AddToolButton(Actions, LOCTEXT("Draft", "存草稿"), 176.f, [this]() { if (Editor.IsValid()) { Editor->SaveDraft(); } });
	AddToolButton(Actions, LOCTEXT("Back", "返回  Esc"), 176.f, [this]() { if (OnBack) { OnBack(); } });

	// Bottom: the status line and the controls.
	UBorder* StatusPanel = SokobanStyle::MakePanel(*WidgetTree, FMargin(28.f, 12.f, 28.f, 18.f));
	Place(Root, StatusPanel, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -12.f));
	UVerticalBox* StatusColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusPanel->SetContent(StatusColumn);
	StatusText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 19, ESokobanFontWeight::Bold);
	StatusText->SetJustification(ETextJustify::Center);
	StatusColumn->AddChildToVerticalBox(StatusText)->SetHorizontalAlignment(HAlign_Center);
	UHorizontalBox* Controls = WidgetTree->ConstructWidget<UHorizontalBox>();
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyPaint", "左键"), LOCTEXT("Paint", "画"), false);
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyErase", "右键"), LOCTEXT("EraseHint", "擦"), false);
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyBrush", "1–8"), LOCTEXT("BrushHint", "换笔刷"), false);
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyUndo", "Ctrl+Z"), LOCTEXT("UndoHint", "撤销"), false);
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyRedo", "Ctrl+Y"), LOCTEXT("RedoHint", "重做"), false);
	SokobanStyle::AddKeyHint(*WidgetTree, *Controls, LOCTEXT("KeyZoom", "滚轮"), LOCTEXT("ZoomHint", "缩放"), false);
	StatusColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	Cast<UVerticalBoxSlot>(Controls->Slot)->SetHorizontalAlignment(HAlign_Center);

	// The panels take clicks; the board around them takes the rest.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StatusPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UBorder* Panel : { InfoPanel, BrushPanel, ActionPanel })
	{
		Panel->OnMouseButtonDownEvent.BindDynamic(this, &USokobanEditorWidget::HandlePanelMouseDown);
	}
}

USokobanButton* USokobanEditorWidget::AddToolButton(UPanelWidget* Column, const FText& Label, float Width, TFunction<void()> Action, const TCHAR* Icon, ESokobanButtonTone Tone)
{
	USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
	Button->InitFocusable(false);
	Button->SetTone(Tone);
	Button->OnActivated = MoveTemp(Action);

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (Icon)
	{
		UImage* Picture = WidgetTree->ConstructWidget<UImage>();
		Picture->SetBrush(SokobanStyle::IconBrush(Icon, 26.f));
		UHorizontalBoxSlot* PictureSlot = Content->AddChildToHorizontalBox(Picture);
		PictureSlot->SetVerticalAlignment(VAlign_Center);
		PictureSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	Content->AddChildToHorizontalBox(SokobanStyle::MakeText(*WidgetTree, Label, Width > 0.f ? 17 : 15, ESokobanFontWeight::Bold))->SetVerticalAlignment(VAlign_Center);
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	if (Width > 0.f)
	{
		Box->SetMinDesiredWidth(Width - 44.f);
	}
	Box->SetContent(Content);
	Button->SetContent(Box);

	Column->AddChild(Button);
	if (UVerticalBoxSlot* ColumnSlot = Cast<UVerticalBoxSlot>(Button->Slot))
	{
		ColumnSlot->SetPadding(FMargin(0.f, 4.f));
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	return Button;
}

void USokobanEditorWidget::Bind(ASokobanLevelEditor* InEditor)
{
	if (Editor.IsValid())
	{
		Editor->OnChanged.Remove(ChangedHandle);
	}
	Editor = InEditor;
	if (InEditor)
	{
		ChangedHandle = InEditor->OnChanged.AddUObject(this, &USokobanEditorWidget::Refresh);
	}
	Refresh();
}

void USokobanEditorWidget::NativeDestruct()
{
	if (Editor.IsValid())
	{
		Editor->OnChanged.Remove(ChangedHandle);
	}
	Super::NativeDestruct();
}

void USokobanEditorWidget::Refresh()
{
	if (!Editor.IsValid())
	{
		return;
	}

	if (!NameBox->HasKeyboardFocus())
	{
		NameBox->SetText(FText::FromString(Editor->GetLevelName()));
	}
	const FIntPoint Size = Editor->GetCanvas().GetSize();
	SizeText->SetText(FText::Format(LOCTEXT("Size", "尺寸 {0} × {1}"), Size.X, Size.Y));

	// The chosen brush keeps the highlighted look.
	for (int32 Index = 0; Index < BrushButtons.Num(); ++Index)
	{
		BrushButtons[Index]->ShowSelected(Index == static_cast<int32>(Editor->GetBrush()));
	}
	UndoButton->SetIsEnabled(Editor->CanUndo());
	RedoButton->SetIsEnabled(Editor->CanRedo());
	TestButton->SetIsEnabled(Editor->CanTestPlay());

	StatusText->SetText(Editor->GetStatus());
	StatusText->SetColorAndOpacity(FSlateColor(Editor->IsStatusProblem() ? SokobanStyle::Warning : SokobanStyle::Ink));
}

bool USokobanEditorWidget::IsEditingName() const
{
	return NameBox && NameBox->HasKeyboardFocus();
}

FEventReply USokobanEditorWidget::HandlePanelMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return FEventReply(true);
}

void USokobanEditorWidget::HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (Editor.IsValid())
	{
		Editor->SetLevelName(Text.ToString());
	}
	Refresh();
	// Hand the keyboard back to the board, so the shortcuts work again.
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

#undef LOCTEXT_NAMESPACE
