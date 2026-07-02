#include "SReSTIRGIDebugPanel.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "SReSTIRGIDebugPanel"

namespace
{
	FText BoolText(const bool bValue)
	{
		return bValue ? LOCTEXT("BoolYes", "Yes") : LOCTEXT("BoolNo", "No");
	}
}

void SReSTIRGIDebugPanel::Construct(const FArguments& InArgs)
{
	Settings = FReSTIRGISettingsManager::GetSettings();

	ModeOptions = {
		MakeShared<FEnumOption>(TEXT("Off"), static_cast<uint8>(EReSTIRGIResamplingMode::Off)),
		MakeShared<FEnumOption>(TEXT("Initial Only"), static_cast<uint8>(EReSTIRGIResamplingMode::InitialOnly)),
		MakeShared<FEnumOption>(TEXT("Temporal"), static_cast<uint8>(EReSTIRGIResamplingMode::Temporal)),
		MakeShared<FEnumOption>(TEXT("Spatial"), static_cast<uint8>(EReSTIRGIResamplingMode::Spatial)),
		MakeShared<FEnumOption>(TEXT("Temporal + Spatial"), static_cast<uint8>(EReSTIRGIResamplingMode::TemporalAndSpatial))
	};

	DebugOptions = {
		MakeShared<FEnumOption>(TEXT("Composite"), static_cast<uint8>(EReSTIRGIDebugView::Composite)),
		MakeShared<FEnumOption>(TEXT("Initial Only"), static_cast<uint8>(EReSTIRGIDebugView::InitialOnly)),
		MakeShared<FEnumOption>(TEXT("Temporal"), static_cast<uint8>(EReSTIRGIDebugView::Temporal)),
		MakeShared<FEnumOption>(TEXT("Spatial"), static_cast<uint8>(EReSTIRGIDebugView::Spatial)),
		MakeShared<FEnumOption>(TEXT("Reservoir Weight"), static_cast<uint8>(EReSTIRGIDebugView::ReservoirWeight)),
		MakeShared<FEnumOption>(TEXT("Sample Age"), static_cast<uint8>(EReSTIRGIDebugView::SampleAge)),
		MakeShared<FEnumOption>(TEXT("Heatmap"), static_cast<uint8>(EReSTIRGIDebugView::Heatmap)),
		MakeShared<FEnumOption>(TEXT("Wipe Compare"), static_cast<uint8>(EReSTIRGIDebugView::WipeCompare)),
		MakeShared<FEnumOption>(TEXT("Side By Side"), static_cast<uint8>(EReSTIRGIDebugView::SideBySide))
	};

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.Text(LOCTEXT("Title", "ReSTIR GI Debug"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Settings.bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					Settings.bEnabled = State == ECheckBoxState::Checked;
					PushSettings();
				})
				[
					SNew(STextBlock).Text(LOCTEXT("Enable", "Enable ReSTIR GI"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return GetStatsText(); })
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.35f)
				[
					SNew(STextBlock).Text(LOCTEXT("Mode", "Mode"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.65f)
				[
					SNew(SComboBox<TSharedPtr<FEnumOption>>)
					.OptionsSource(&ModeOptions)
					.OnGenerateWidget(this, &SReSTIRGIDebugPanel::MakeModeComboWidget)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FEnumOption> Option, ESelectInfo::Type)
					{
						if (Option.IsValid())
						{
							Settings.ResamplingMode = static_cast<EReSTIRGIResamplingMode>(Option->Value);
							PushSettings();
						}
					})
					[
						SNew(STextBlock).Text(this, &SReSTIRGIDebugPanel::GetModeText)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f)[SNew(STextBlock).Text(LOCTEXT("DebugView", "Debug View"))]
				+ SHorizontalBox::Slot().FillWidth(0.65f)
				[
					SNew(SComboBox<TSharedPtr<FEnumOption>>)
					.OptionsSource(&DebugOptions)
					.OnGenerateWidget(this, &SReSTIRGIDebugPanel::MakeDebugComboWidget)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FEnumOption> Option, ESelectInfo::Type)
					{
						if (Option.IsValid())
						{
							Settings.DebugView = static_cast<EReSTIRGIDebugView>(Option->Value);
							PushSettings();
						}
					})
					[
						SNew(STextBlock).Text(this, &SReSTIRGIDebugPanel::GetDebugText)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(32)
				.Value_Lambda([this]() { return Settings.SpatialSampleCount; })
				.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
				{
					Settings.SpatialSampleCount = Value;
					PushSettings();
				})
				.ToolTipText(LOCTEXT("SpatialSampleCountTip", "Spatial samples per pixel"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(1.0f)
				.MaxValue(128.0f)
				.Value_Lambda([this]() { return Settings.SpatialRadius; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
				{
					Settings.SpatialRadius = Value;
					PushSettings();
				})
				.ToolTipText(LOCTEXT("SpatialRadiusTip", "Spatial sampling radius in pixels"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(255)
				.Value_Lambda([this]() { return Settings.MaxHistoryLength; })
				.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
				{
					Settings.MaxHistoryLength = Value;
					PushSettings();
				})
				.ToolTipText(LOCTEXT("HistoryLengthTip", "Maximum temporal reservoir age"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(1.0f)
				.Value_Lambda([this]() { return Settings.NormalThreshold; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
				{
					Settings.NormalThreshold = Value;
					PushSettings();
				})
				.ToolTipText(LOCTEXT("NormalThresholdTip", "Normal similarity threshold"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.01f)
				.MaxValue(10.0f)
				.Value_Lambda([this]() { return Settings.RadianceClamp; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
				{
					Settings.RadianceClamp = Value;
					PushSettings();
				})
				.ToolTipText(LOCTEXT("RadianceClampTip", "Reservoir radiance clamp"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SSlider)
				.Value_Lambda([this]() { return Settings.CompareSplit; })
				.OnValueChanged_Lambda([this](float Value)
				{
					Settings.CompareSplit = Value;
					PushSettings();
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFinalMIS ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						Settings.bFinalMIS = State == ECheckBoxState::Checked;
						PushSettings();
					})
					[
						SNew(STextBlock).Text(LOCTEXT("MIS", "MIS"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFinalVisibility ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						Settings.bFinalVisibility = State == ECheckBoxState::Checked;
						PushSettings();
					})
					[
						SNew(STextBlock).Text(LOCTEXT("Visibility", "Visibility"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFreezeHistory ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						Settings.bFreezeHistory = State == ECheckBoxState::Checked;
						PushSettings();
					})
					[
						SNew(STextBlock).Text(LOCTEXT("FreezeHistory", "Freeze History"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 12, 0, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ResetHistory", "Reset History"))
				.OnClicked_Lambda([]()
				{
					FReSTIRGISettingsManager::RequestHistoryReset();
					return FReply::Handled();
				})
			]
		]
	];
}

void SReSTIRGIDebugPanel::PushSettings() const
{
	FReSTIRGISettingsManager::UpdateSettings(Settings);
}

TSharedRef<SWidget> SReSTIRGIDebugPanel::MakeModeComboWidget(TSharedPtr<FEnumOption> Option) const
{
	return SNew(STextBlock).Text(Option.IsValid() ? FText::FromString(Option->Label) : FText::GetEmpty());
}

TSharedRef<SWidget> SReSTIRGIDebugPanel::MakeDebugComboWidget(TSharedPtr<FEnumOption> Option) const
{
	return SNew(STextBlock).Text(Option.IsValid() ? FText::FromString(Option->Label) : FText::GetEmpty());
}

FText SReSTIRGIDebugPanel::GetModeText() const
{
	for (const TSharedPtr<FEnumOption>& Option : ModeOptions)
	{
		if (Option.IsValid() && Option->Value == static_cast<uint8>(Settings.ResamplingMode))
		{
			return FText::FromString(Option->Label);
		}
	}
	return LOCTEXT("UnknownMode", "Unknown");
}

FText SReSTIRGIDebugPanel::GetDebugText() const
{
	for (const TSharedPtr<FEnumOption>& Option : DebugOptions)
	{
		if (Option.IsValid() && Option->Value == static_cast<uint8>(Settings.DebugView))
		{
			return FText::FromString(Option->Label);
		}
	}
	return LOCTEXT("UnknownDebugView", "Unknown");
}

FText SReSTIRGIDebugPanel::GetStatsText() const
{
	const FReSTIRGIRuntimeStats Stats = FReSTIRGISettingsManager::GetStats();
	return FText::Format(
		LOCTEXT("Stats", "View: {0} x {1}\nReservoir memory: {2} KB\nEstimated rays/pixel: {3}\nHistory valid: {4}\nView extension active: {5}"),
		FText::AsNumber(Stats.ViewSize.X),
		FText::AsNumber(Stats.ViewSize.Y),
		FText::AsNumber(Stats.ReservoirBytes / 1024),
		FText::AsNumber(Stats.EstimatedRaysPerPixel),
		BoolText(Stats.bHistoryValid),
		BoolText(Stats.bViewExtensionActive));
}

#undef LOCTEXT_NAMESPACE
