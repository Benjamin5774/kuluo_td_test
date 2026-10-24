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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "SokobanEditorWidget"

namespace
{
	const FLinearColor Warning(1.f, 0.47f, 0.36f);

	UBorder* MakePanel(UWidgetTree& Tree, const FMargin& Padding)
	{
		UBorder* Panel = Tree.ConstructWidget<UBorder>();
		Panel->SetBrush(SokobanStyle::RoundedBrush(SokobanStyle::Panel, 14.f));
		Panel->SetPadding(Padding);
		return Panel;
	}

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
	UBorder* InfoPanel = MakePanel(*WidgetTree, FMargin(20.f, 16.f));
	Place(Root, InfoPanel, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(24.f, 24.f));
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>();
	InfoPanel->SetContent(Info);
	Info->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Title", "关卡编辑器"), 26, true, SokobanStyle::Accent));

	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Info->AddChildToVerticalBox(NameRow)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	UHorizontalBoxSlot* NameLabelSlot = NameRow->AddChildToHorizontalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Name", "名称"), 16));
	NameLabelSlot->SetVerticalAlignment(VAlign_Center);
	NameLabelSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	NameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	FEditableTextBoxStyle NameStyle;
	NameStyle.SetFont(SokobanStyle::Font(18))
		.SetForegroundColor(FSlateColor(SokobanStyle::Text))
		.SetBackgroundImageNormal(SokobanStyle::RoundedBrush(SokobanStyle::ButtonNormal, 8.f))
		.SetBackgroundImageHovered(SokobanStyle::RoundedBrush(SokobanStyle::ButtonNormal, 8.f))
		.SetBackgroundImageFocused(SokobanStyle::RoundedBrush(FLinearColor(0.2f, 0.21f, 0.26f), 8.f))
		.SetPadding(FMargin(10.f, 6.f));
	NameBox->WidgetStyle = NameStyle;
	NameBox->OnTextCommitted.AddDynamic(this, &USokobanEditorWidget::HandleNameCommitted);
	USizeBox* NameWidth = WidgetTree->ConstructWidget<USizeBox>();
	NameWidth->SetWidthOverride(250.f);
	NameWidth->SetContent(NameBox);
	NameRow->AddChildToHorizontalBox(NameWidth);

	UHorizontalBox* SizeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Info->AddChildToVerticalBox(SizeRow)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	SizeText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 16);
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

	// Left: the brushes, in ESokobanBrush order, with their number keys.
	UBorder* BrushPanel = MakePanel(*WidgetTree, FMargin(14.f));
	Place(Root, BrushPanel, FAnchors(0.f, 0.5f), FVector2D(0.f, 0.5f), FVector2D(24.f, 60.f));
	UVerticalBox* Brushes = WidgetTree->ConstructWidget<UVerticalBox>();
	BrushPanel->SetContent(Brushes);
	Brushes->AddChildToVerticalBox(SokobanStyle::MakeText(*WidgetTree, LOCTEXT("Brushes", "笔刷"), 16, true, SokobanStyle::TextDim))->SetPadding(FMargin(4.f, 0.f, 0.f, 6.f));
	const FText BrushLabels[] = {
		LOCTEXT("Floor", "1  地面"), LOCTEXT("Wall", "2  墙"), LOCTEXT("Crate", "3  箱子"),
		LOCTEXT("Goal", "4  目标点"), LOCTEXT("Hero", "5  大橘"), LOCTEXT("Plate", "6  机关"),
		LOCTEXT("Trapdoor", "7  活板门"), LOCTEXT("Erase", "8  擦除") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BrushLabels); ++Index)
	{
		const ESokobanBrush Brush = static_cast<ESokobanBrush>(Index);
		BrushButtons.Add(AddToolButton(Brushes, BrushLabels[Index], 150.f, [this, Brush]() { if (Editor.IsValid()) { Editor->SetBrush(Brush); } }));
	}

	// Right: actions.
	UBorder* ActionPanel = MakePanel(*WidgetTree, FMargin(14.f));
	Place(Root, ActionPanel, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-24.f, 40.f));
	UVerticalBox* Actions = WidgetTree->ConstructWidget<UVerticalBox>();
	ActionPanel->SetContent(Actions);
	UndoButton = AddToolButton(Actions, LOCTEXT("Undo", "撤销  Ctrl+Z"), 170.f, [this]() { if (Editor.IsValid()) { Editor->Undo(); } });
	RedoButton = AddToolButton(Actions, LOCTEXT("Redo", "重做  Ctrl+Y"), 170.f, [this]() { if (Editor.IsValid()) { Editor->Redo(); } });
	AddToolButton(Actions, LOCTEXT("WallIn", "自动围墙"), 170.f, [this]() { if (Editor.IsValid()) { Editor->WallIn(); } });
	AddToolButton(Actions, LOCTEXT("Check", "检查可解"), 170.f, [this]() { if (Editor.IsValid()) { Editor->CheckSolvable(); } });
	TestButton = AddToolButton(Actions, LOCTEXT("Test", "试玩"), 170.f, [this]() { if (OnTestPlay) { OnTestPlay(); } });
	AddToolButton(Actions, LOCTEXT("Save", "保存关卡"), 170.f, [this]() { if (Editor.IsValid()) { Editor->SaveLevel(); } });
	AddToolButton(Actions, LOCTEXT("Draft", "存草稿"), 170.f, [this]() { if (Editor.IsValid()) { Editor->SaveDraft(); } });
	AddToolButton(Actions, LOCTEXT("Back", "返回  Esc"), 170.f, [this]() { if (OnBack) { OnBack(); } });

	// Bottom: the status line and the controls.
	UBorder* StatusPanel = MakePanel(*WidgetTree, FMargin(24.f, 10.f));
	Place(Root, StatusPanel, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -18.f));
	UVerticalBox* StatusColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusPanel->SetContent(StatusColumn);
	StatusText = SokobanStyle::MakeText(*WidgetTree, FText::GetEmpty(), 19, true, SokobanStyle::Accent);
	StatusText->SetJustification(ETextJustify::Center);
	StatusColumn->AddChildToVerticalBox(StatusText)->SetHorizontalAlignment(HAlign_Center);
	UTextBlock* Controls = SokobanStyle::MakeText(*WidgetTree,
		LOCTEXT("Controls", "左键画    右键擦    1–8 换笔刷    Ctrl+Z 撤销    Ctrl+Y 重做    滚轮 缩放"), 14, false, SokobanStyle::TextDim);
	Controls->SetJustification(ETextJustify::Center);
	StatusColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	// The panels take clicks; the board around them takes the rest.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StatusPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UBorder* Panel : { InfoPanel, BrushPanel, ActionPanel })
	{
		Panel->OnMouseButtonDownEvent.BindDynamic(this, &USokobanEditorWidget::HandlePanelMouseDown);
	}
}

USokobanButton* USokobanEditorWidget::AddToolButton(UPanelWidget* Column, const FText& Label, float Width, TFunction<void()> Action)
{
	USokobanButton* Button = WidgetTree->ConstructWidget<USokobanButton>();
	Button->InitFocusable(false);
	Button->SetStyle(SokobanStyle::ButtonStyle());
	Button->OnActivated = MoveTemp(Action);

	UTextBlock* Text = SokobanStyle::MakeText(*WidgetTree, Label, Width > 0.f ? 17 : 15, true);
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	if (Width > 0.f)
	{
		Box->SetMinDesiredWidth(Width - 48.f);
	}
	Box->SetContent(Text);
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
		BrushButtons[Index]->SetStyle(SokobanStyle::ButtonStyle(Index == static_cast<int32>(Editor->GetBrush())));
	}
	UndoButton->SetIsEnabled(Editor->CanUndo());
	RedoButton->SetIsEnabled(Editor->CanRedo());
	TestButton->SetIsEnabled(Editor->CanTestPlay());

	StatusText->SetText(Editor->GetStatus());
	StatusText->SetColorAndOpacity(FSlateColor(Editor->IsStatusProblem() ? Warning : SokobanStyle::Accent));
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
