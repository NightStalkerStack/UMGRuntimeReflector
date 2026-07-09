#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "IDetailsView.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class ITableRow;
class SSearchBox;
class STableViewBase;
class SUMGReflectorFrame;
class UGameViewportClient;
class UUserWidget;
class UWidget;

enum class EUMGReflectorNodeType : uint8
{
	GameLayerRoot,
	UserWidget,
	Widget
};

struct FUMGReflectorNode : public TSharedFromThis<FUMGReflectorNode>
{
	EUMGReflectorNodeType Type = EUMGReflectorNodeType::Widget;
	TWeakObjectPtr<UWidget> Widget;
	TWeakObjectPtr<UUserWidget> RootUserWidget;
	TWeakPtr<FUMGReflectorNode> Parent;
	TArray<TSharedPtr<FUMGReflectorNode>> Children;
	TArray<TSharedPtr<FUMGReflectorNode>> VisibleChildren;
	FString DisplayName;
	FString ClassName;
	int32 Depth = 0;

	UObject* GetObject() const;
	bool IsGameLayerRoot() const;
};

class FUMGReflectorInputProcessor : public IInputProcessor
{
public:
	explicit FUMGReflectorInputProcessor(class SUMGRuntimeReflector* InOwner);

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual const TCHAR* GetDebugName() const override;

private:
	SUMGRuntimeReflector* Owner = nullptr;
};

class SUMGRuntimeReflector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUMGRuntimeReflector) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	~SUMGRuntimeReflector();

	void ExitCapture();
	void HandleGlobalMouseMove(const FVector2D& ScreenSpacePosition, bool bAltDown);

private:
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<ITableRow> GenerateTreeRow(TSharedPtr<FUMGReflectorNode> InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void GetNodeChildren(TSharedPtr<FUMGReflectorNode> InNode, TArray<TSharedPtr<FUMGReflectorNode>>& OutChildren) const;
	void HandleTreeSelectionChanged(TSharedPtr<FUMGReflectorNode> InNode, ESelectInfo::Type SelectInfo);

	FReply HandleRefreshClicked();
	FReply HandleCaptureClicked();
	void HandleSearchTextChanged(const FText& InText);
	void HandleShowFrameChanged(ECheckBoxState InState);
	ECheckBoxState GetShowFrameState() const;
	void HandleNodeVisibilityChanged(ECheckBoxState InState, TSharedPtr<FUMGReflectorNode> InNode);
	ECheckBoxState GetNodeVisibilityCheckState(TSharedPtr<FUMGReflectorNode> InNode) const;
	FText GetCaptureButtonText() const;
	FSlateColor GetCaptureButtonColor() const;

	void RefreshAllWidgets();
	void RebuildFromUserWidgets(const TArray<UUserWidget*>& RootWidgets, UWidget* WidgetToSelect);
	TSharedPtr<FUMGReflectorNode> BuildGameLayerRoot(const TArray<UUserWidget*>& RootWidgets);
	TSharedPtr<FUMGReflectorNode> BuildUserWidgetNode(UUserWidget* InUserWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParent, int32 InDepth, bool bTopLevel);
	TSharedPtr<FUMGReflectorNode> BuildWidgetNode(UWidget* InWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParent, int32 InDepth, TSet<UWidget*>& VisitedWidgets);
	void BuildWidgetChildren(UWidget* InWidget, UUserWidget* InRootUserWidget, TSharedPtr<FUMGReflectorNode> InParentNode, int32 ChildDepth, TSet<UWidget*>& VisitedWidgets);

	void ApplySearchFilter();
	bool ApplySearchFilterRecursive(TSharedPtr<FUMGReflectorNode> InNode);
	bool DoesNodeMatchSearch(TSharedPtr<FUMGReflectorNode> InNode) const;
	void ExpandVisibleNodes(TSharedPtr<FUMGReflectorNode> InNode);
	TSharedPtr<FUMGReflectorNode> FindNodeForWidget(UWidget* InWidget) const;
	TSharedPtr<FUMGReflectorNode> FindNodeForWidgetRecursive(TSharedPtr<FUMGReflectorNode> InNode, UWidget* InWidget) const;

	void SetSelectedWidget(UWidget* InWidget, bool bForceDetailsRefresh);
	void SetDetailsObject(UObject* InObject, bool bForceRefresh);
	void OpenBlueprintForNode(TSharedPtr<FUMGReflectorNode> InNode) const;
	UObject* ResolveBlueprintObjectForNode(TSharedPtr<FUMGReflectorNode> InNode) const;

	void SetCaptureActive(bool bInActive);
	void RegisterInputProcessor();
	void UnregisterInputProcessor();
	void HandleEndPIE(bool bWasSimulatingInEditor);

	void UpdateFrameWidget();
	void RemoveFrameWidget();
	UGameViewportClient* GetViewportClientForWidget(UWidget* InWidget) const;

	UWorld* FindPIEWorldUnderCursor(const FWidgetPath& WidgetPath) const;
	TArray<UUserWidget*> GetTopLevelUserWidgets(UWorld* InWorld) const;
	bool IsWidgetUsable(UWidget* InWidget) const;

	bool CaptureByTunnel(const FVector2D& ScreenSpacePosition, UWorld* InWorld, UWidget*& OutWidget, UUserWidget*& OutRootUserWidget) const;
	bool CaptureByDeepest(const FVector2D& ScreenSpacePosition, UWorld* InWorld, UWidget*& OutWidget, UUserWidget*& OutRootUserWidget) const;
	UWidget* FindWidgetBySlateWidget(UUserWidget* InRootUserWidget, TSharedRef<SWidget> InSlateWidget) const;
	UWidget* FindWidgetBySlateWidgetRecursive(UWidget* InWidget, TSharedRef<SWidget> InSlateWidget, TSet<UWidget*>& VisitedWidgets) const;
	UUserWidget* FindTopLevelRootForWidget(UWorld* InWorld, UWidget* InWidget) const;
	bool DoesRootContainWidget(UUserWidget* InRootUserWidget, UWidget* InWidget) const;
	bool DoesWidgetContainWidgetRecursive(UWidget* InCurrentWidget, UWidget* InTargetWidget, TSet<UWidget*>& VisitedWidgets) const;
	void FindDeepestWidgetRecursive(UWidget* InWidget, UUserWidget* InRootUserWidget, const FVector2D& ScreenSpacePosition, int32 Depth, UWidget*& OutBestWidget, UUserWidget*& OutBestRoot, int32& OutBestDepth, float& OutBestArea, TSet<UWidget*>& VisitedWidgets) const;

private:
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STreeView<TSharedPtr<FUMGReflectorNode>>> TreeView;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<FUMGReflectorNode> GameLayerRootNode;
	TArray<TSharedPtr<FUMGReflectorNode>> VisibleRootNodes;

	TWeakObjectPtr<UWidget> SelectedWidget;
	TWeakObjectPtr<UObject> DetailsObject;
	TWeakObjectPtr<UWidget> LastCapturedWidget;
	TWeakObjectPtr<UUserWidget> LastCapturedRoot;

	TSharedPtr<SUMGReflectorFrame> FrameWidget;
	TWeakObjectPtr<UGameViewportClient> FrameViewportClient;

	TSharedPtr<FUMGReflectorInputProcessor> InputProcessor;
	FDelegateHandle EndPIEHandle;

	FString SearchText;
	bool bCaptureActive = false;
	bool bShowFrame = true;
	double LastCaptureTime = 0.0;
};
