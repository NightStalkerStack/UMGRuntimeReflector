#include "SUMGRuntimeReflector.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NamedSlotInterface.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "DetailLayoutBuilder.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailCustomization.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "Slate/SObjectWidget.h"
#include "Styling/SlateIconFinder.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "SUMGReflectorFrame.h"
#include "UMGReflectorCanvasSlotCustomization.h"
#include "UMGReflectorSlotPropertyCustomizations.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SUMGRuntimeReflector"

namespace
{
FString GetWidgetClassName(const UWidget* Widget)
{
	if (!Widget || !Widget->GetClass())
	{
		return FString();
	}

	FString ClassName = Widget->GetClass()->GetName();
	if (ClassName.EndsWith(TEXT("_C")))
	{
		ClassName = ClassName.LeftChop(2);
	}

	return ClassName;
}

FString GetWidgetDisplayName(UWidget* Widget, bool bTopLevel)
{
	if (!Widget)
	{
		return FString();
	}

	if (bTopLevel && Widget->IsA<UUserWidget>())
	{
		return GetWidgetClassName(Widget);
	}

	return Widget->GetName();
}

bool IsVisibleForPicking(const UWidget* Widget)
{
	if (!Widget)
	{
		return false;
	}

	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
}

void AddUniqueChild(UWidget* ChildWidget, TArray<UWidget*>& OutChildren, TSet<UWidget*>& AddedChildren)
{
	if (ChildWidget && !AddedChildren.Contains(ChildWidget))
	{
		AddedChildren.Add(ChildWidget);
		OutChildren.Add(ChildWidget);
	}
}

void GatherDirectUMGChildren(UWidget* Widget, TArray<UWidget*>& OutChildren)
{
	TSet<UWidget*> AddedChildren;

	if (INamedSlotInterface* NamedSlotHost = Cast<INamedSlotInterface>(Widget))
	{
		TArray<FName> SlotNames;
		NamedSlotHost->GetSlotNames(SlotNames);

		for (const FName SlotName : SlotNames)
		{
			AddUniqueChild(NamedSlotHost->GetContentForSlot(SlotName), OutChildren, AddedChildren);
		}
	}

	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
		{
			AddUniqueChild(PanelWidget->GetChildAt(ChildIndex), OutChildren, AddedChildren);
		}
	}
}

TSet<UWidget*> MakeWidgetSet(const TArray<UWidget*>& Widgets)
{
	TSet<UWidget*> Result;
	for (UWidget* Widget : Widgets)
	{
		if (Widget)
		{
			Result.Add(Widget);
		}
	}
	return Result;
}

class FUMGReflectorWidgetDetailsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FUMGReflectorWidgetDetailsCustomization>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		DetailBuilder.EditCategory(TEXT("Layout"), FText::GetEmpty(), ECategoryPriority::Transform);
	}
};
}

UObject* FUMGReflectorNode::GetObject() const
{
	return Widget.Get();
}

bool FUMGReflectorNode::IsGameLayerRoot() const
{
	return Type == EUMGReflectorNodeType::GameLayerRoot;
}

FUMGReflectorInputProcessor::FUMGReflectorInputProcessor(SUMGRuntimeReflector* InOwner)
	: Owner(InOwner)
{
}

void FUMGReflectorInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool FUMGReflectorInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (Owner && InKeyEvent.GetKey() == EKeys::Escape)
	{
		Owner->ExitCapture();
		return true;
	}

	return false;
}

bool FUMGReflectorInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (Owner)
	{
		Owner->HandleGlobalMouseMove(MouseEvent.GetScreenSpacePosition(), MouseEvent.IsAltDown());
	}

	return false;
}

const TCHAR* FUMGReflectorInputProcessor::GetDebugName() const
{
	return TEXT("UMGRuntimeReflectorInputProcessor");
}

void SUMGRuntimeReflector::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	FDetailsViewArgs DetailsViewArgs(
		false,
		false,
		true,
		FDetailsViewArgs::HideNameArea,
		true);
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowSearch = true;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->RegisterInstancedCustomPropertyLayout(UWidget::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FUMGReflectorWidgetDetailsCustomization::MakeInstance));
	DetailsView->RegisterInstancedCustomPropertyTypeLayout(TEXT("PanelSlot"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUMGReflectorCanvasSlotCustomization::MakeInstance));
	DetailsView->RegisterInstancedCustomPropertyTypeLayout(TEXT("EHorizontalAlignment"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUMGReflectorHorizontalAlignmentCustomization::MakeInstance));
	DetailsView->RegisterInstancedCustomPropertyTypeLayout(TEXT("EVerticalAlignment"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUMGReflectorVerticalAlignmentCustomization::MakeInstance));
	DetailsView->RegisterInstancedCustomPropertyTypeLayout(TEXT("SlateChildSize"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUMGReflectorSlateChildSizeCustomization::MakeInstance));

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.45f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildToolbar()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("NoBorder")))
				.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
				[
					SAssignNew(TreeView, STreeView<TSharedPtr<FUMGReflectorNode>>)
					.TreeItemsSource(&VisibleRootNodes)
					.OnGenerateRow(this, &SUMGRuntimeReflector::GenerateTreeRow)
					.OnGetChildren(this, &SUMGRuntimeReflector::GetNodeChildren)
					.OnSelectionChanged(this, &SUMGRuntimeReflector::HandleTreeSelectionChanged)
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.55f)
		[
			BuildRightPanel()
		]
	];

	EndPIEHandle = FEditorDelegates::EndPIE.AddSP(this, &SUMGRuntimeReflector::HandleEndPIE);
	RefreshAllWidgets();
}

SUMGRuntimeReflector::~SUMGRuntimeReflector()
{
	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	}

	UnregisterInputProcessor();
	RemoveFrameWidget();
}

void SUMGRuntimeReflector::ExitCapture()
{
	SetCaptureActive(false);
}

void SUMGRuntimeReflector::HandleGlobalMouseMove(const FVector2D& ScreenSpacePosition, bool bAltDown)
{
	if (!bCaptureActive || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastCaptureTime < 0.03)
	{
		return;
	}
	LastCaptureTime = CurrentTime;

	FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(
		ScreenSpacePosition,
		FSlateApplication::Get().GetInteractiveTopLevelWindows(),
		true);

	UWorld* PIEWorld = FindPIEWorldUnderCursor(WidgetPath);
	if (!PIEWorld)
	{
		return;
	}

	UWidget* CapturedWidget = nullptr;
	UUserWidget* CapturedRoot = nullptr;
	const bool bCaptured = bAltDown
		? CaptureByDeepest(ScreenSpacePosition, PIEWorld, CapturedWidget, CapturedRoot)
		: CaptureByTunnel(ScreenSpacePosition, PIEWorld, CapturedWidget, CapturedRoot);

	if (!bCaptured || !CapturedWidget || !CapturedRoot)
	{
		return;
	}

	if (LastCapturedWidget.Get() == CapturedWidget && LastCapturedRoot.Get() == CapturedRoot)
	{
		return;
	}

	LastCapturedWidget = CapturedWidget;
	LastCapturedRoot = CapturedRoot;

	TArray<UUserWidget*> RootWidgets;
	RootWidgets.Add(CapturedRoot);
	RebuildFromUserWidgets(RootWidgets, CapturedWidget);
}

TSharedRef<SWidget> SUMGRuntimeReflector::BuildToolbar()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(FMargin(4.0f, 3.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(220.0f)
				[
					SAssignNew(SearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchHint", "搜索控件变量名"))
					.OnTextChanged(this, &SUMGRuntimeReflector::HandleSearchTextChanged)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshButton", "刷新"))
				.OnClicked(this, &SUMGRuntimeReflector::HandleRefreshClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(this, &SUMGRuntimeReflector::GetCaptureButtonText)
				.ButtonColorAndOpacity(this, &SUMGRuntimeReflector::GetCaptureButtonColor)
				.OnClicked(this, &SUMGRuntimeReflector::HandleCaptureClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SUMGRuntimeReflector::GetShowFrameState)
				.OnCheckStateChanged(this, &SUMGRuntimeReflector::HandleShowFrameChanged)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowFrameCheckBox", "显示Frame"))
				]
			]
		];
}

TSharedRef<SWidget> SUMGRuntimeReflector::BuildRightPanel()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.BorderBackgroundColor(FSlateColor(FLinearColor(0.105f, 0.105f, 0.105f, 1.0f)))
		.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f)))
				.Padding(FMargin(8.0f, 3.0f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DetailsTab", "DetailsView"))
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
			[
				DetailsView.ToSharedRef()
			]
		];
}

TSharedRef<ITableRow> SUMGRuntimeReflector::GenerateTreeRow(TSharedPtr<FUMGReflectorNode> InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	const UWidget* Widget = InNode.IsValid() ? InNode->Widget.Get() : nullptr;
	const FSlateBrush* IconBrush = Widget && Widget->GetClass()
		? FSlateIconFinder::FindIconBrushForClass(Widget->GetClass(), TEXT("ClassIcon.Default"))
		: FEditorStyle::GetBrush(TEXT("ClassIcon.Default"));
	if (!IconBrush)
	{
		IconBrush = FEditorStyle::GetBrush(TEXT("ClassIcon.Default"));
	}

	TSharedRef<SWidget> VisibilityWidget = Widget
		? StaticCastSharedRef<SWidget>(
			SNew(SCheckBox)
			.IsChecked(this, &SUMGRuntimeReflector::GetNodeVisibilityCheckState, InNode)
			.OnCheckStateChanged(this, &SUMGRuntimeReflector::HandleNodeVisibilityChanged, InNode))
		: StaticCastSharedRef<SWidget>(
			SNew(SBox)
			.WidthOverride(16.0f));

	TSharedRef<SHorizontalBox> RowContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			VisibilityWidget
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SImage)
			.Image(IconBrush)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(InNode.IsValid() ? InNode->DisplayName : FString()))
		];

	if (InNode.IsValid() && !InNode->ClassName.IsEmpty() && ResolveBlueprintObjectForNode(InNode))
	{
		RowContent->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(8.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SHyperlink)
			.Text(FText::FromString(InNode->ClassName))
			.OnNavigate(FSimpleDelegate::CreateSP(this, &SUMGRuntimeReflector::OpenBlueprintForNode, InNode))
		];
	}
	else if (InNode.IsValid() && !InNode->ClassName.IsEmpty())
	{
		RowContent->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(8.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(InNode->ClassName))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	}

	return SNew(STableRow<TSharedPtr<FUMGReflectorNode>>, OwnerTable)
		[
			RowContent
		];
}

void SUMGRuntimeReflector::GetNodeChildren(TSharedPtr<FUMGReflectorNode> InNode, TArray<TSharedPtr<FUMGReflectorNode>>& OutChildren) const
{
	if (InNode.IsValid())
	{
		OutChildren.Append(InNode->VisibleChildren);
	}
}

void SUMGRuntimeReflector::HandleTreeSelectionChanged(TSharedPtr<FUMGReflectorNode> InNode, ESelectInfo::Type SelectInfo)
{
	UWidget* Widget = InNode.IsValid() ? InNode->Widget.Get() : nullptr;
	SelectedWidget = Widget;
	SetDetailsObject(Widget, false);
	UpdateFrameWidget();
}

FReply SUMGRuntimeReflector::HandleRefreshClicked()
{
	RefreshAllWidgets();
	return FReply::Handled();
}

FReply SUMGRuntimeReflector::HandleCaptureClicked()
{
	SetCaptureActive(!bCaptureActive);
	return FReply::Handled();
}

void SUMGRuntimeReflector::HandleSearchTextChanged(const FText& InText)
{
	SearchText = InText.ToString();
	ApplySearchFilter();
}

void SUMGRuntimeReflector::HandleShowFrameChanged(ECheckBoxState InState)
{
	bShowFrame = InState == ECheckBoxState::Checked;
	UpdateFrameWidget();
}

ECheckBoxState SUMGRuntimeReflector::GetShowFrameState() const
{
	return bShowFrame ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SUMGRuntimeReflector::HandleNodeVisibilityChanged(ECheckBoxState InState, TSharedPtr<FUMGReflectorNode> InNode)
{
	if (!InNode.IsValid())
	{
		return;
	}

	UWidget* Widget = InNode->Widget.Get();
	if (!Widget)
	{
		return;
	}

	// 树节点的勾选状态直接驱动运行时控件可见性，方便快速观察布局变化。
	Widget->SetVisibility(InState == ECheckBoxState::Checked ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (DetailsObject.Get() == Widget)
	{
		SetDetailsObject(Widget, true);
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}

	UpdateFrameWidget();
}

ECheckBoxState SUMGRuntimeReflector::GetNodeVisibilityCheckState(TSharedPtr<FUMGReflectorNode> InNode) const
{
	UWidget* Widget = InNode.IsValid() ? InNode->Widget.Get() : nullptr;
	if (!Widget)
	{
		return ECheckBoxState::Unchecked;
	}

	const ESlateVisibility vis = Widget->GetVisibility();
	return vis == ESlateVisibility::Collapsed || vis == ESlateVisibility::Hidden
		? ECheckBoxState::Unchecked
		: ECheckBoxState::Checked;
}

FText SUMGRuntimeReflector::GetCaptureButtonText() const
{
	return bCaptureActive ? LOCTEXT("ExitCaptureButton", "退出捕获") : LOCTEXT("CaptureButton", "实时捕获");
}

FSlateColor SUMGRuntimeReflector::GetCaptureButtonColor() const
{
	return bCaptureActive
		? FSlateColor(FLinearColor(0.1f, 0.45f, 1.0f, 1.0f))
		: FSlateColor(FLinearColor::White);
}

void SUMGRuntimeReflector::RefreshAllWidgets()
{
	TArray<UUserWidget*> RootWidgets;

	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (WorldContext.WorldType == EWorldType::PIE && World)
			{
				RootWidgets.Append(GetTopLevelUserWidgets(World));
			}
		}
	}

	RebuildFromUserWidgets(RootWidgets, SelectedWidget.Get());
}

void SUMGRuntimeReflector::RebuildFromUserWidgets(const TArray<UUserWidget*>& RootWidgets, UWidget* WidgetToSelect)
{
	GameLayerRootNode.Reset();
	VisibleRootNodes.Empty();

	if (RootWidgets.Num() > 0)
	{
		GameLayerRootNode = BuildGameLayerRoot(RootWidgets);
	}

	ApplySearchFilter();

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();

		for (TSharedPtr<FUMGReflectorNode> RootNode : VisibleRootNodes)
		{
			ExpandVisibleNodes(RootNode);
		}
	}

	if (WidgetToSelect)
	{
		SetSelectedWidget(WidgetToSelect, true);
	}
	else
	{
		SelectedWidget.Reset();
		SetDetailsObject(nullptr, true);
		UpdateFrameWidget();
	}
}

TSharedPtr<FUMGReflectorNode> SUMGRuntimeReflector::BuildGameLayerRoot(const TArray<UUserWidget*>& RootWidgets)
{
	TSharedPtr<FUMGReflectorNode> RootNode = MakeShared<FUMGReflectorNode>();
	RootNode->Type = EUMGReflectorNodeType::GameLayerRoot;
	RootNode->DisplayName = TEXT("GameViewport");
	RootNode->Depth = 0;

	TSet<UUserWidget*> AddedRoots;
	for (UUserWidget* RootWidget : RootWidgets)
	{
		if (!RootWidget || AddedRoots.Contains(RootWidget))
		{
			continue;
		}

		AddedRoots.Add(RootWidget);
		TSharedPtr<FUMGReflectorNode> UserWidgetNode = BuildUserWidgetNode(RootWidget, RootWidget, RootNode, 1, true);
		if (UserWidgetNode.IsValid())
		{
			RootNode->Children.Add(UserWidgetNode);
		}
	}

	return RootNode;
}

TSharedPtr<FUMGReflectorNode> SUMGRuntimeReflector::BuildUserWidgetNode(UUserWidget* InUserWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParent, int32 InDepth, bool bTopLevel)
{
	if (!IsWidgetUsable(InUserWidget))
	{
		return nullptr;
	}

	TSharedPtr<FUMGReflectorNode> Node = MakeShared<FUMGReflectorNode>();
	Node->Type = EUMGReflectorNodeType::UserWidget;
	Node->Widget = InUserWidget;
	Node->RootUserWidget = InRootUserWidget ? InRootUserWidget : InUserWidget;
	Node->Parent = InParent;
	Node->DisplayName = GetWidgetDisplayName(InUserWidget, bTopLevel);
	Node->ClassName = GetWidgetClassName(InUserWidget);
	Node->Depth = InDepth;

	TSet<UWidget*> VisitedWidgets;
	VisitedWidgets.Add(InUserWidget);
	BuildWidgetChildren(InUserWidget, Node->RootUserWidget.Get(), Node, InDepth + 1, VisitedWidgets);

	return Node;
}

TSharedPtr<FUMGReflectorNode> SUMGRuntimeReflector::BuildWidgetNode(UWidget* InWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParent, int32 InDepth, TSet<UWidget*>& VisitedWidgets)
{
	if (!IsWidgetUsable(InWidget))
	{
		return nullptr;
	}

	TSharedPtr<FUMGReflectorNode> Node = MakeShared<FUMGReflectorNode>();
	UUserWidget* UserWidget = Cast<UUserWidget>(InWidget);
	Node->Type = UserWidget ? EUMGReflectorNodeType::UserWidget : EUMGReflectorNodeType::Widget;
	Node->Widget = InWidget;
	Node->RootUserWidget = UserWidget ? UserWidget : InRootUserWidget;
	Node->Parent = InParent;
	Node->DisplayName = GetWidgetDisplayName(InWidget, false);
	Node->ClassName = Node->Type == EUMGReflectorNodeType::UserWidget
		? GetWidgetClassName(InWidget)
		: GetWidgetClassName(Node->RootUserWidget.Get());
	Node->Depth = InDepth;

	BuildWidgetChildren(InWidget, Node->RootUserWidget.Get(), Node, InDepth + 1, VisitedWidgets);
	return Node;
}

void SUMGRuntimeReflector::BuildWidgetChildren(UWidget* InWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParentNode, int32 ChildDepth, TSet<UWidget*>& VisitedWidgets)
{
	if (!InWidget || !InParentNode.IsValid())
	{
		return;
	}

	TArray<UWidget*> Children;
	GatherDirectUMGChildren(InWidget, Children);

	if (UUserWidget* UserWidget = Cast<UUserWidget>(InWidget))
	{
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
		{
			TSet<UWidget*> AddedChildren = MakeWidgetSet(Children);
			AddUniqueChild(UserWidget->WidgetTree->RootWidget, Children, AddedChildren);
		}
	}

	for (UWidget* ChildWidget : Children)
	{
		if (!ChildWidget || VisitedWidgets.Contains(ChildWidget))
		{
			continue;
		}

		VisitedWidgets.Add(ChildWidget);
		TSharedPtr<FUMGReflectorNode> ChildNode = BuildWidgetNode(ChildWidget, InRootUserWidget, InParentNode, ChildDepth, VisitedWidgets);
		if (ChildNode.IsValid())
		{
			InParentNode->Children.Add(ChildNode);
		}
	}
}

void SUMGRuntimeReflector::ApplySearchFilter()
{
	VisibleRootNodes.Empty();

	if (GameLayerRootNode.IsValid() && ApplySearchFilterRecursive(GameLayerRootNode))
	{
		VisibleRootNodes.Add(GameLayerRootNode);
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
		for (TSharedPtr<FUMGReflectorNode> RootNode : VisibleRootNodes)
		{
			ExpandVisibleNodes(RootNode);
		}
	}
}

bool SUMGRuntimeReflector::ApplySearchFilterRecursive(TSharedPtr<FUMGReflectorNode> InNode)
{
	if (!InNode.IsValid())
	{
		return false;
	}

	InNode->VisibleChildren.Empty();

	const bool bSearchEmpty = SearchText.IsEmpty();
	bool bHasVisibleChild = false;

	for (TSharedPtr<FUMGReflectorNode> ChildNode : InNode->Children)
	{
		if (ApplySearchFilterRecursive(ChildNode))
		{
			InNode->VisibleChildren.Add(ChildNode);
			bHasVisibleChild = true;
		}
	}

	return bSearchEmpty || DoesNodeMatchSearch(InNode) || bHasVisibleChild;
}

bool SUMGRuntimeReflector::DoesNodeMatchSearch(TSharedPtr<FUMGReflectorNode> InNode) const
{
	if (!InNode.IsValid() || SearchText.IsEmpty() || InNode->IsGameLayerRoot())
	{
		return false;
	}

	return InNode->DisplayName.Contains(SearchText, ESearchCase::IgnoreCase, ESearchDir::FromStart);
}

void SUMGRuntimeReflector::ExpandVisibleNodes(TSharedPtr<FUMGReflectorNode> InNode)
{
	if (!TreeView.IsValid() || !InNode.IsValid())
	{
		return;
	}

	TreeView->SetItemExpansion(InNode, true);
	for (TSharedPtr<FUMGReflectorNode> ChildNode : InNode->VisibleChildren)
	{
		ExpandVisibleNodes(ChildNode);
	}
}

TSharedPtr<FUMGReflectorNode> SUMGRuntimeReflector::FindNodeForWidget(UWidget* InWidget) const
{
	return FindNodeForWidgetRecursive(GameLayerRootNode, InWidget);
}

TSharedPtr<FUMGReflectorNode> SUMGRuntimeReflector::FindNodeForWidgetRecursive(TSharedPtr<FUMGReflectorNode> InNode, UWidget* InWidget) const
{
	if (!InNode.IsValid() || !InWidget)
	{
		return nullptr;
	}

	if (InNode->Widget.Get() == InWidget)
	{
		return InNode;
	}

	for (TSharedPtr<FUMGReflectorNode> ChildNode : InNode->Children)
	{
		TSharedPtr<FUMGReflectorNode> FoundNode = FindNodeForWidgetRecursive(ChildNode, InWidget);
		if (FoundNode.IsValid())
		{
			return FoundNode;
		}
	}

	return nullptr;
}

void SUMGRuntimeReflector::SetSelectedWidget(UWidget* InWidget, bool bForceDetailsRefresh)
{
	SelectedWidget = InWidget;

	TSharedPtr<FUMGReflectorNode> NodeToSelect = FindNodeForWidget(InWidget);

	if (TreeView.IsValid() && NodeToSelect.IsValid())
	{
		TSharedPtr<FUMGReflectorNode> ParentNode = NodeToSelect->Parent.Pin();
		while (ParentNode.IsValid())
		{
			TreeView->SetItemExpansion(ParentNode, true);
			ParentNode = ParentNode->Parent.Pin();
		}

		TreeView->SetSelection(NodeToSelect, ESelectInfo::Direct);
		TreeView->RequestScrollIntoView(NodeToSelect);
	}

	SetDetailsObject(InWidget, bForceDetailsRefresh);
	UpdateFrameWidget();
}

void SUMGRuntimeReflector::SetDetailsObject(UObject* InObject, bool bForceRefresh)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	if (!bForceRefresh && DetailsObject.Get() == InObject)
	{
		return;
	}

	DetailsObject = InObject;
	DetailsView->SetObject(InObject, bForceRefresh);
}

void SUMGRuntimeReflector::OpenBlueprintForNode(TSharedPtr<FUMGReflectorNode> InNode) const
{
	UObject* BlueprintObject = ResolveBlueprintObjectForNode(InNode);
	if (BlueprintObject && GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(BlueprintObject);
		}
	}
}

UObject* SUMGRuntimeReflector::ResolveBlueprintObjectForNode(TSharedPtr<FUMGReflectorNode> InNode) const
{
	if (!InNode.IsValid() || InNode->IsGameLayerRoot())
	{
		return nullptr;
	}

	UWidget* Widget = InNode->Widget.Get();
	UClass* ClassToOpen = nullptr;

	if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		ClassToOpen = UserWidget->GetClass();
	}
	else if (UUserWidget* RootUserWidget = InNode->RootUserWidget.Get())
	{
		ClassToOpen = RootUserWidget->GetClass();
	}

	return ClassToOpen ? UBlueprint::GetBlueprintFromClass(ClassToOpen) : nullptr;
}

void SUMGRuntimeReflector::SetCaptureActive(bool bInActive)
{
	if (bCaptureActive == bInActive)
	{
		return;
	}

	bCaptureActive = bInActive;
	LastCapturedWidget.Reset();
	LastCapturedRoot.Reset();

	if (bCaptureActive)
	{
		RegisterInputProcessor();
	}
	else
	{
		UnregisterInputProcessor();
	}
}

void SUMGRuntimeReflector::RegisterInputProcessor()
{
	if (!InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FUMGReflectorInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
	}
}

void SUMGRuntimeReflector::UnregisterInputProcessor()
{
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}

	InputProcessor.Reset();
}

void SUMGRuntimeReflector::HandleEndPIE(bool bWasSimulatingInEditor)
{
	SetCaptureActive(false);
	SelectedWidget.Reset();
	LastCapturedWidget.Reset();
	LastCapturedRoot.Reset();
	GameLayerRootNode.Reset();
	VisibleRootNodes.Empty();

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}

	SetDetailsObject(nullptr, true);
	RemoveFrameWidget();
}

void SUMGRuntimeReflector::UpdateFrameWidget()
{
	UWidget* Widget = SelectedWidget.Get();
	if (!bShowFrame || !Widget)
	{
		if (FrameWidget.IsValid())
		{
			FrameWidget->SetTargetWidget(nullptr);
			FrameWidget->SetFrameVisible(false);
		}
		return;
	}

	UGameViewportClient* ViewportClient = GetViewportClientForWidget(Widget);
	if (!ViewportClient)
	{
		return;
	}

	if (FrameViewportClient.Get() != ViewportClient)
	{
		RemoveFrameWidget();
	}

	if (!FrameWidget.IsValid())
	{
		SAssignNew(FrameWidget, SUMGReflectorFrame);
		ViewportClient->AddViewportWidgetContent(FrameWidget.ToSharedRef(), 10000);
		FrameViewportClient = ViewportClient;
	}

	FrameWidget->SetTargetWidget(Widget);
	FrameWidget->SetFrameVisible(true);
}

void SUMGRuntimeReflector::RemoveFrameWidget()
{
	if (FrameWidget.IsValid() && FrameViewportClient.IsValid())
	{
		FrameViewportClient->RemoveViewportWidgetContent(FrameWidget.ToSharedRef());
	}

	FrameWidget.Reset();
	FrameViewportClient.Reset();
}

UGameViewportClient* SUMGRuntimeReflector::GetViewportClientForWidget(UWidget* InWidget) const
{
	if (!InWidget || !GEngine)
	{
		return nullptr;
	}

	UWorld* WidgetWorld = InWidget->GetWorld();
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.World() == WidgetWorld)
		{
			return WorldContext.GameViewport;
		}
	}

	return nullptr;
}

UWorld* SUMGRuntimeReflector::FindPIEWorldUnderCursor(const FWidgetPath& WidgetPath) const
{
	if (!WidgetPath.IsValid() || !GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE || !WorldContext.World() || !WorldContext.GameViewport)
		{
			continue;
		}

		TSharedPtr<SViewport> GameViewportWidget = WorldContext.GameViewport->GetGameViewportWidget();
		if (GameViewportWidget.IsValid() && WidgetPath.ContainsWidget(GameViewportWidget.ToSharedRef()))
		{
			return WorldContext.World();
		}
	}

	return nullptr;
}

TArray<UUserWidget*> SUMGRuntimeReflector::GetTopLevelUserWidgets(UWorld* InWorld) const
{
	TArray<UUserWidget*> Result;

	if (!InWorld)
	{
		return Result;
	}

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* UserWidget = *It;
		if (!IsWidgetUsable(UserWidget) || UserWidget->GetWorld() != InWorld || !UserWidget->IsInViewport())
		{
			continue;
		}

		Result.Add(UserWidget);
	}

	return Result;
}

bool SUMGRuntimeReflector::IsWidgetUsable(UWidget* InWidget) const
{
	return IsValid(InWidget) && !InWidget->IsTemplate() && !InWidget->HasAnyFlags(RF_ClassDefaultObject);
}

bool SUMGRuntimeReflector::CaptureByTunnel(const FVector2D& ScreenSpacePosition, UWorld* InWorld, UWidget*& OutWidget, UUserWidget*& OutRootUserWidget) const
{
	OutWidget = nullptr;
	OutRootUserWidget = nullptr;

	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(
		ScreenSpacePosition,
		FSlateApplication::Get().GetInteractiveTopLevelWindows(),
		true);

	if (!WidgetPath.IsValid())
	{
		return false;
	}

	TArray<UUserWidget*> RootWidgets = GetTopLevelUserWidgets(InWorld);

	for (int32 PathIndex = WidgetPath.Widgets.Num() - 1; PathIndex >= 0; --PathIndex)
	{
		TSharedRef<SWidget> SlateWidget = WidgetPath.Widgets[PathIndex].Widget;

		for (UUserWidget* RootWidget : RootWidgets)
		{
			if (UWidget* FoundWidget = FindWidgetBySlateWidget(RootWidget, SlateWidget))
			{
				OutWidget = FoundWidget;
				OutRootUserWidget = FindTopLevelRootForWidget(InWorld, FoundWidget);
				return OutRootUserWidget != nullptr;
			}
		}

		if (SlateWidget->GetTypeAsString() == TEXT("SObjectWidget"))
		{
			TSharedRef<SObjectWidget> ObjectWidget = StaticCastSharedRef<SObjectWidget>(SlateWidget);
			if (UUserWidget* UserWidget = ObjectWidget->GetWidgetObject())
			{
				if (UserWidget->GetWorld() == InWorld)
				{
					OutWidget = UserWidget;
					OutRootUserWidget = FindTopLevelRootForWidget(InWorld, UserWidget);
					return OutRootUserWidget != nullptr;
				}
			}
		}
	}

	return false;
}

bool SUMGRuntimeReflector::CaptureByDeepest(const FVector2D& ScreenSpacePosition, UWorld* InWorld, UWidget*& OutWidget, UUserWidget*& OutRootUserWidget) const
{
	OutWidget = nullptr;
	OutRootUserWidget = nullptr;

	int32 BestDepth = INDEX_NONE;
	float BestArea = TNumericLimits<float>::Max();

	for (UUserWidget* RootWidget : GetTopLevelUserWidgets(InWorld))
	{
		TSet<UWidget*> VisitedWidgets;
		FindDeepestWidgetRecursive(RootWidget, RootWidget, ScreenSpacePosition, 0, OutWidget, OutRootUserWidget, BestDepth, BestArea, VisitedWidgets);
	}

	return OutWidget != nullptr && OutRootUserWidget != nullptr;
}

UWidget* SUMGRuntimeReflector::FindWidgetBySlateWidget(UUserWidget* InRootUserWidget, TSharedRef<SWidget> InSlateWidget) const
{
	if (!InRootUserWidget)
	{
		return nullptr;
	}

	TSet<UWidget*> VisitedWidgets;
	return FindWidgetBySlateWidgetRecursive(InRootUserWidget, InSlateWidget, VisitedWidgets);
}

UWidget* SUMGRuntimeReflector::FindWidgetBySlateWidgetRecursive(UWidget* InWidget, TSharedRef<SWidget> InSlateWidget, TSet<UWidget*>& VisitedWidgets) const
{
	if (!IsWidgetUsable(InWidget) || VisitedWidgets.Contains(InWidget))
	{
		return nullptr;
	}

	VisitedWidgets.Add(InWidget);

	if (InWidget->GetCachedWidget() == InSlateWidget)
	{
		return InWidget;
	}

	TArray<UWidget*> Children;
	GatherDirectUMGChildren(InWidget, Children);

	if (UUserWidget* UserWidget = Cast<UUserWidget>(InWidget))
	{
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
		{
			TSet<UWidget*> AddedChildren = MakeWidgetSet(Children);
			AddUniqueChild(UserWidget->WidgetTree->RootWidget, Children, AddedChildren);
		}
	}

	for (UWidget* ChildWidget : Children)
	{
		if (UWidget* FoundWidget = FindWidgetBySlateWidgetRecursive(ChildWidget, InSlateWidget, VisitedWidgets))
		{
			return FoundWidget;
		}
	}

	return nullptr;
}

UUserWidget* SUMGRuntimeReflector::FindTopLevelRootForWidget(UWorld* InWorld, UWidget* InWidget) const
{
	if (!InWidget)
	{
		return nullptr;
	}

	for (UUserWidget* RootWidget : GetTopLevelUserWidgets(InWorld))
	{
		if (DoesRootContainWidget(RootWidget, InWidget))
		{
			return RootWidget;
		}
	}

	return nullptr;
}

bool SUMGRuntimeReflector::DoesRootContainWidget(UUserWidget* InRootUserWidget, UWidget* InWidget) const
{
	if (!InRootUserWidget || !InWidget)
	{
		return false;
	}

	TSet<UWidget*> VisitedWidgets;
	return DoesWidgetContainWidgetRecursive(InRootUserWidget, InWidget, VisitedWidgets);
}

bool SUMGRuntimeReflector::DoesWidgetContainWidgetRecursive(UWidget* InCurrentWidget, UWidget* InTargetWidget, TSet<UWidget*>& VisitedWidgets) const
{
	if (!IsWidgetUsable(InCurrentWidget) || VisitedWidgets.Contains(InCurrentWidget))
	{
		return false;
	}

	VisitedWidgets.Add(InCurrentWidget);

	if (InCurrentWidget == InTargetWidget)
	{
		return true;
	}

	TArray<UWidget*> Children;
	GatherDirectUMGChildren(InCurrentWidget, Children);

	if (UUserWidget* UserWidget = Cast<UUserWidget>(InCurrentWidget))
	{
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
		{
			TSet<UWidget*> AddedChildren = MakeWidgetSet(Children);
			AddUniqueChild(UserWidget->WidgetTree->RootWidget, Children, AddedChildren);
		}
	}

	for (UWidget* ChildWidget : Children)
	{
		if (DoesWidgetContainWidgetRecursive(ChildWidget, InTargetWidget, VisitedWidgets))
		{
			return true;
		}
	}

	return false;
}

void SUMGRuntimeReflector::FindDeepestWidgetRecursive(UWidget* InWidget, UUserWidget* InRootUserWidget, const FVector2D& ScreenSpacePosition, int32 Depth, UWidget*& OutBestWidget, UUserWidget*& OutBestRoot, int32& OutBestDepth, float& OutBestArea, TSet<UWidget*>& VisitedWidgets) const
{
	if (!IsWidgetUsable(InWidget) || VisitedWidgets.Contains(InWidget))
	{
		return;
	}

	VisitedWidgets.Add(InWidget);

	if (IsVisibleForPicking(InWidget))
	{
		const FGeometry& Geometry = InWidget->GetCachedGeometry();
		const FVector2D AbsoluteSize = Geometry.GetAbsoluteSize();
		const float Area = AbsoluteSize.X * AbsoluteSize.Y;

		if (Area > 1.0f && Geometry.IsUnderLocation(ScreenSpacePosition))
		{
			if (Depth > OutBestDepth || (Depth == OutBestDepth && Area < OutBestArea))
			{
				OutBestWidget = InWidget;
				OutBestRoot = InRootUserWidget;
				OutBestDepth = Depth;
				OutBestArea = Area;
			}
		}
	}

	TArray<UWidget*> Children;
	GatherDirectUMGChildren(InWidget, Children);

	if (UUserWidget* UserWidget = Cast<UUserWidget>(InWidget))
	{
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
		{
			TSet<UWidget*> AddedChildren = MakeWidgetSet(Children);
			AddUniqueChild(UserWidget->WidgetTree->RootWidget, Children, AddedChildren);
		}
	}

	for (UWidget* ChildWidget : Children)
	{
		FindDeepestWidgetRecursive(ChildWidget, InRootUserWidget, ScreenSpacePosition, Depth + 1, OutBestWidget, OutBestRoot, OutBestDepth, OutBestArea, VisitedWidgets);
	}
}

#undef LOCTEXT_NAMESPACE
