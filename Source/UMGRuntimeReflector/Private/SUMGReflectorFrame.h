#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class UWidget;

class SUMGReflectorFrame : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUMGReflectorFrame) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetTargetWidget(UWidget* InWidget);
	void SetFrameVisible(bool bInVisible);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TWeakObjectPtr<UWidget> TargetWidget;
	bool bFrameVisible = false;
};
