#include "SUMGReflectorFrame.h"

#include "Components/Widget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SUMGReflectorFrame::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::HitTestInvisible);
	ForceVolatile(true);
}

void SUMGReflectorFrame::SetTargetWidget(UWidget* InWidget)
{
	TargetWidget = InWidget;
	Invalidate(EInvalidateWidget::Paint);
}

void SUMGReflectorFrame::SetFrameVisible(bool bInVisible)
{
	bFrameVisible = bInVisible;
	Invalidate(EInvalidateWidget::Paint);
}

int32 SUMGReflectorFrame::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	UWidget* Widget = TargetWidget.Get();
	if (!bFrameVisible || !Widget)
	{
		return LayerId;
	}

	const ESlateVisibility WidgetVisibility = Widget->GetVisibility();
	if (WidgetVisibility == ESlateVisibility::Collapsed || WidgetVisibility == ESlateVisibility::Hidden)
	{
		return LayerId;
	}

	const FGeometry& TargetGeometry = Widget->GetPaintSpaceGeometry();
	const FSlateRect TargetRect = TargetGeometry.GetRenderBoundingRect();
	const FVector2D AbsolutePosition(TargetRect.Left, TargetRect.Top);
	const FVector2D AbsoluteSize(TargetRect.GetSize());

	if (AbsoluteSize.X <= 1.0f || AbsoluteSize.Y <= 1.0f)
	{
		return LayerId;
	}

	// Slate绘制层使用本控件的局部坐标，因此需要把目标控件的屏幕坐标转换回来。
	const FVector2D RawLocalMin = AllottedGeometry.AbsoluteToLocal(AbsolutePosition);
	const FVector2D RawLocalMax = AllottedGeometry.AbsoluteToLocal(AbsolutePosition + AbsoluteSize);
	const FVector2D LocalMin(FMath::RoundToFloat(RawLocalMin.X), FMath::RoundToFloat(RawLocalMin.Y));
	const FVector2D LocalMax(FMath::RoundToFloat(RawLocalMax.X), FMath::RoundToFloat(RawLocalMax.Y));
	const FVector2D LocalSize = LocalMax - LocalMin;

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(LocalMin, LocalSize),
		FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
		ESlateDrawEffect::None,
		FLinearColor(1.0f, 0.85f, 0.0f, 0.08f));

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FLinearColor FrameColor(1.0f, 0.85f, 0.0f, 1.0f);
	const float Thickness = 3.0f;

	const FVector2D HorizontalSize(LocalSize.X, Thickness);
	const FVector2D VerticalSize(Thickness, LocalSize.Y);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(LocalMin, HorizontalSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		FrameColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(LocalMin.X, LocalMax.Y - Thickness), HorizontalSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		FrameColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(LocalMin, VerticalSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		FrameColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(LocalMax.X - Thickness, LocalMin.Y), VerticalSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		FrameColor);

	return LayerId + 2;
}

FVector2D SUMGReflectorFrame::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D::ZeroVector;
}
