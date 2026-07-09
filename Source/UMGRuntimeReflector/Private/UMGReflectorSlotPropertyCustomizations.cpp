#include "UMGReflectorSlotPropertyCustomizations.h"

#include "DetailWidgetRow.h"
#include "EditorStyleSet.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UMG"

void FUMGReflectorHorizontalAlignmentCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const FMargin OuterPadding(2);
	const FMargin ContentPadding(2);

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("HAlignLeft", "Horizontally Align Left"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorHorizontalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, HAlign_Left)
			.IsChecked(this, &FUMGReflectorHorizontalAlignmentCustomization::GetCheckState, PropertyHandle, HAlign_Left)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("HorizontalAlignment_Left"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("HAlignCenter", "Horizontally Align Center"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorHorizontalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, HAlign_Center)
			.IsChecked(this, &FUMGReflectorHorizontalAlignmentCustomization::GetCheckState, PropertyHandle, HAlign_Center)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("HorizontalAlignment_Center"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("HAlignRight", "Horizontally Align Right"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorHorizontalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, HAlign_Right)
			.IsChecked(this, &FUMGReflectorHorizontalAlignmentCustomization::GetCheckState, PropertyHandle, HAlign_Right)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("HorizontalAlignment_Right"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("HAlignFill", "Horizontally Align Fill"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorHorizontalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, HAlign_Fill)
			.IsChecked(this, &FUMGReflectorHorizontalAlignmentCustomization::GetCheckState, PropertyHandle, HAlign_Fill)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("HorizontalAlignment_Fill"))
			]
		]
	];
}

void FUMGReflectorHorizontalAlignmentCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FUMGReflectorHorizontalAlignmentCustomization::HandleCheckStateChanged(ECheckBoxState InCheckboxState, TSharedRef<IPropertyHandle> PropertyHandle, EHorizontalAlignment ToAlignment)
{
	PropertyHandle->SetValue(static_cast<uint8>(ToAlignment));
}

ECheckBoxState FUMGReflectorHorizontalAlignmentCustomization::GetCheckState(TSharedRef<IPropertyHandle> PropertyHandle, EHorizontalAlignment ForAlignment) const
{
	uint8 Value = 0;
	if (PropertyHandle->GetValue(Value) == FPropertyAccess::Result::Success)
	{
		return Value == ForAlignment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void FUMGReflectorVerticalAlignmentCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const FMargin OuterPadding(2);
	const FMargin ContentPadding(2);

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("VAlignTop", "Vertically Align Top"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorVerticalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, VAlign_Top)
			.IsChecked(this, &FUMGReflectorVerticalAlignmentCustomization::GetCheckState, PropertyHandle, VAlign_Top)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("VerticalAlignment_Top"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("VAlignCenter", "Vertically Align Center"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorVerticalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, VAlign_Center)
			.IsChecked(this, &FUMGReflectorVerticalAlignmentCustomization::GetCheckState, PropertyHandle, VAlign_Center)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("VerticalAlignment_Center"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("VAlignBottom", "Vertically Align Bottom"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorVerticalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, VAlign_Bottom)
			.IsChecked(this, &FUMGReflectorVerticalAlignmentCustomization::GetCheckState, PropertyHandle, VAlign_Bottom)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("VerticalAlignment_Bottom"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("VAlignFill", "Vertically Align Fill"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FUMGReflectorVerticalAlignmentCustomization::HandleCheckStateChanged, PropertyHandle, VAlign_Fill)
			.IsChecked(this, &FUMGReflectorVerticalAlignmentCustomization::GetCheckState, PropertyHandle, VAlign_Fill)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("VerticalAlignment_Fill"))
			]
		]
	];
}

void FUMGReflectorVerticalAlignmentCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FUMGReflectorVerticalAlignmentCustomization::HandleCheckStateChanged(ECheckBoxState InCheckboxState, TSharedRef<IPropertyHandle> PropertyHandle, EVerticalAlignment ToAlignment)
{
	PropertyHandle->SetValue(static_cast<uint8>(ToAlignment));
}

ECheckBoxState FUMGReflectorVerticalAlignmentCustomization::GetCheckState(TSharedRef<IPropertyHandle> PropertyHandle, EVerticalAlignment ForAlignment) const
{
	uint8 Value = 0;
	if (PropertyHandle->GetValue(Value) == FPropertyAccess::Result::Success)
	{
		return Value == ForAlignment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void FUMGReflectorSlateChildSizeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSlateChildSize, Value));
	TSharedPtr<IPropertyHandle> RuleHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSlateChildSize, SizeRule));

	const FMargin OuterPadding(2, 0);
	const FMargin ContentPadding(4, 2);

	if (!ValueHandle.IsValid() || !RuleHandle.IsValid())
	{
		return;
	}

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(TOptional<float>())
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(OuterPadding)

			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SCheckBox)
				.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
				.ToolTipText(LOCTEXT("Auto_ToolTip", "Only requests as much room as it needs based on the widgets desired size."))
				.Padding(ContentPadding)
				.OnCheckStateChanged(this, &FUMGReflectorSlateChildSizeCustomization::HandleCheckStateChanged, RuleHandle, ESlateSizeRule::Automatic)
				.IsChecked(this, &FUMGReflectorSlateChildSizeCustomization::GetCheckState, RuleHandle, ESlateSizeRule::Automatic)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Auto", "Auto"))
				]
			]

			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SCheckBox)
				.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
				.ToolTipText(LOCTEXT("Fill_ToolTip", "Greedily attempts to fill all available room based on the percentage value 0..1"))
				.Padding(ContentPadding)
				.OnCheckStateChanged(this, &FUMGReflectorSlateChildSizeCustomization::HandleCheckStateChanged, RuleHandle, ESlateSizeRule::Fill)
				.IsChecked(this, &FUMGReflectorSlateChildSizeCustomization::GetCheckState, RuleHandle, ESlateSizeRule::Fill)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Fill", "Fill"))
				]
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SBox)
			.WidthOverride(40)
			[
				SNew(SNumericEntryBox<float>)
				.LabelVAlign(VAlign_Center)
				.Visibility(this, &FUMGReflectorSlateChildSizeCustomization::GetValueVisibility, RuleHandle)
				.Value(this, &FUMGReflectorSlateChildSizeCustomization::GetValue, ValueHandle)
				.OnValueCommitted(this, &FUMGReflectorSlateChildSizeCustomization::HandleValueCommitted, ValueHandle)
				.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			]
		]
	];
}

void FUMGReflectorSlateChildSizeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FUMGReflectorSlateChildSizeCustomization::HandleCheckStateChanged(ECheckBoxState InCheckboxState, TSharedPtr<IPropertyHandle> PropertyHandle, ESlateSizeRule::Type ToRule)
{
	if (PropertyHandle.IsValid())
	{
		PropertyHandle->SetValue(static_cast<uint8>(ToRule));
	}
}

ECheckBoxState FUMGReflectorSlateChildSizeCustomization::GetCheckState(TSharedPtr<IPropertyHandle> PropertyHandle, ESlateSizeRule::Type ForRule) const
{
	uint8 Value = 0;
	if (PropertyHandle.IsValid() && PropertyHandle->GetValue(Value) == FPropertyAccess::Result::Success)
	{
		return Value == ForRule ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

TOptional<float> FUMGReflectorSlateChildSizeCustomization::GetValue(TSharedPtr<IPropertyHandle> ValueHandle) const
{
	float Value = 0.0f;
	if (ValueHandle.IsValid() && ValueHandle->GetValue(Value) == FPropertyAccess::Result::Success)
	{
		return Value;
	}

	return TOptional<float>();
}

void FUMGReflectorSlateChildSizeCustomization::HandleValueCommitted(float NewValue, ETextCommit::Type CommitType, TSharedPtr<IPropertyHandle> ValueHandle)
{
	if (ValueHandle.IsValid())
	{
		ValueHandle->SetValue(NewValue);
	}
}

EVisibility FUMGReflectorSlateChildSizeCustomization::GetValueVisibility(TSharedPtr<IPropertyHandle> RuleHandle) const
{
	uint8 Value = 0;
	if (RuleHandle.IsValid() && RuleHandle->GetValue(Value) == FPropertyAccess::Result::Success)
	{
		return Value == ESlateSizeRule::Fill ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
