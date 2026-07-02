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

	FText RatioText(const float Ratio)
	{
		if (Ratio < 0.0f)
		{
			return LOCTEXT("StatsNA", "N/A");
		}
		return FText::AsPercent(Ratio);
	}

	FText FloatText(const float Value)
	{
		if (Value < 0.0f)
		{
			return LOCTEXT("StatsNA", "N/A");
		}
		return FText::AsNumber(Value);
	}
}

void SReSTIRGIDebugPanel::Construct(const FArguments& InArgs)
{
	Settings = FReSTIRGISettingsManager::GetSettings();

	ModeOptions = {
		MakeShared<FEnumOption>(TEXT("关闭"), static_cast<uint8>(EReSTIRGIResamplingMode::Off)),
		MakeShared<FEnumOption>(TEXT("初始采样"), static_cast<uint8>(EReSTIRGIResamplingMode::InitialOnly)),
		MakeShared<FEnumOption>(TEXT("时间复用"), static_cast<uint8>(EReSTIRGIResamplingMode::Temporal)),
		MakeShared<FEnumOption>(TEXT("空间复用"), static_cast<uint8>(EReSTIRGIResamplingMode::Spatial)),
		MakeShared<FEnumOption>(TEXT("时间+空间复用（推荐）"), static_cast<uint8>(EReSTIRGIResamplingMode::TemporalAndSpatial))
	};

	DebugOptions = {
		MakeShared<FEnumOption>(TEXT("最终合成"), static_cast<uint8>(EReSTIRGIDebugView::Composite)),
		MakeShared<FEnumOption>(TEXT("仅看间接光（ToneMapped）"), static_cast<uint8>(EReSTIRGIDebugView::IndirectOnly)),
		MakeShared<FEnumOption>(TEXT("仅看间接光（Linear）"), static_cast<uint8>(EReSTIRGIDebugView::IndirectOnlyLinear)),
		MakeShared<FEnumOption>(TEXT("仅看间接光（Tonemapped）"), static_cast<uint8>(EReSTIRGIDebugView::IndirectOnlyTonemapped)),
		MakeShared<FEnumOption>(TEXT("二次射线命中"), static_cast<uint8>(EReSTIRGIDebugView::HitMask)),
		MakeShared<FEnumOption>(TEXT("表面有效性"), static_cast<uint8>(EReSTIRGIDebugView::SurfaceValidity)),
		MakeShared<FEnumOption>(TEXT("Initial Reservoir"), static_cast<uint8>(EReSTIRGIDebugView::InitialReservoir)),
		MakeShared<FEnumOption>(TEXT("Temporal Reservoir"), static_cast<uint8>(EReSTIRGIDebugView::TemporalReservoir)),
		MakeShared<FEnumOption>(TEXT("Spatial Reservoir"), static_cast<uint8>(EReSTIRGIDebugView::SpatialReservoir)),
		MakeShared<FEnumOption>(TEXT("Reservoir 权重"), static_cast<uint8>(EReSTIRGIDebugView::ReservoirWeight)),
		MakeShared<FEnumOption>(TEXT("历史年龄"), static_cast<uint8>(EReSTIRGIDebugView::SampleAge)),
		MakeShared<FEnumOption>(TEXT("选中样本方向"), static_cast<uint8>(EReSTIRGIDebugView::SelectedSample)),
		MakeShared<FEnumOption>(TEXT("TargetPdf"), static_cast<uint8>(EReSTIRGIDebugView::TargetPdf)),
		MakeShared<FEnumOption>(TEXT("WeightSum/M"), static_cast<uint8>(EReSTIRGIDebugView::WeightSumM)),
		MakeShared<FEnumOption>(TEXT("Indirect Raw"), static_cast<uint8>(EReSTIRGIDebugView::IndirectRaw))
	};

	LumenOptions = {
		MakeShared<FEnumOption>(TEXT("叠加显示"), static_cast<uint8>(EReSTIRGILumenCompareMode::Overlay)),
		MakeShared<FEnumOption>(TEXT("UE Direct + ReSTIR GI"), static_cast<uint8>(EReSTIRGILumenCompareMode::ReSTIROnly)),
		MakeShared<FEnumOption>(TEXT("只看 UE/Lumen"), static_cast<uint8>(EReSTIRGILumenCompareMode::LumenOnly)),
		MakeShared<FEnumOption>(TEXT("滑杆擦除对比"), static_cast<uint8>(EReSTIRGILumenCompareMode::Wipe)),
		MakeShared<FEnumOption>(TEXT("左右分屏对比"), static_cast<uint8>(EReSTIRGILumenCompareMode::SideBySide))
	};

	RadianceOptions = {
		MakeShared<FEnumOption>(TEXT("Synthetic Constant（验证用）"), static_cast<uint8>(EReSTIRGIRadianceMode::SyntheticConstant)),
		MakeShared<FEnumOption>(TEXT("Normal Color（验证用）"), static_cast<uint8>(EReSTIRGIRadianceMode::NormalColor)),
		MakeShared<FEnumOption>(TEXT("Albedo Color（验证用）"), static_cast<uint8>(EReSTIRGIRadianceMode::AlbedoColor)),
		MakeShared<FEnumOption>(TEXT("Screen Projected SceneColor"), static_cast<uint8>(EReSTIRGIRadianceMode::ScreenProjected))
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
				.Text(LOCTEXT("Title", "ReSTIR GI 调试面板"))
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
					SNew(STextBlock).Text(LOCTEXT("Enable", "启用 ReSTIR GI"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return GetStatsText(); })
			]
			// --- Algorithm Mode ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f)[SNew(STextBlock).Text(LOCTEXT("Mode", "算法模式"))]
				+ SHorizontalBox::Slot().FillWidth(0.65f)
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
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ModeHint", "算法模式只决定 final reservoir 来源。先用\"初始采样\"看噪声，再用\"时间+空间复用\"看稳定结果。"))
			]
			// --- Debug View ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f)[SNew(STextBlock).Text(LOCTEXT("DebugView", "诊断视图"))]
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
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DebugHint", "诊断视图用于验证算法各阶段。屏幕对比请使用下面的 Lumen 对比。debug 视图下不触发 wipe/side-by-side。"))
			]
			// --- Radiance Mode ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f)[SNew(STextBlock).Text(LOCTEXT("RadianceMode", "辐射模式"))]
				+ SHorizontalBox::Slot().FillWidth(0.65f)
				[
					SNew(SComboBox<TSharedPtr<FEnumOption>>)
					.OptionsSource(&RadianceOptions)
					.OnGenerateWidget(this, &SReSTIRGIDebugPanel::MakeRadianceComboWidget)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FEnumOption> Option, ESelectInfo::Type)
					{
						if (Option.IsValid())
						{
							Settings.RadianceMode = static_cast<EReSTIRGIRadianceMode>(Option->Value);
							PushSettings();
						}
					})
					[
						SNew(STextBlock).Text(this, &SReSTIRGIDebugPanel::GetRadianceText)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RadianceHint", "推荐先用 Synthetic Constant 验证算法闭环，再切回 Screen Projected 观察真实近似效果。"))
			]
			// --- Lumen Compare ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f)[SNew(STextBlock).Text(LOCTEXT("LumenCompare", "Lumen 对比"))]
				+ SHorizontalBox::Slot().FillWidth(0.65f)
				[
					SNew(SComboBox<TSharedPtr<FEnumOption>>)
					.OptionsSource(&LumenOptions)
					.OnGenerateWidget(this, &SReSTIRGIDebugPanel::MakeLumenComboWidget)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FEnumOption> Option, ESelectInfo::Type)
					{
						if (Option.IsValid())
						{
							Settings.LumenCompareMode = static_cast<EReSTIRGILumenCompareMode>(Option->Value);
							FReSTIRGISettingsManager::RequestHistoryReset();
							PushSettings();
						}
					})
					[
						SNew(STextBlock).Text(this, &SReSTIRGIDebugPanel::GetLumenText)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CompareHint", "Lumen 对比只负责最终画面对比。\"UE Direct + ReSTIR GI\"会临时关闭 Lumen，不写入项目配置。"))
			]
			// --- Basic Parameters ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 12, 0, 4)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.Text(LOCTEXT("BasicSection", "基础参数（优先调这些）"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1).MaxValue(32)
				.Value_Lambda([this]() { return Settings.SpatialSampleCount; })
				.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type) { Settings.SpatialSampleCount = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("SpatialSampleCountTip", "空间复用时每像素合并的邻居数量。推荐 4。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(1.0f).MaxValue(128.0f)
				.Value_Lambda([this]() { return Settings.SpatialRadius; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.SpatialRadius = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("SpatialRadiusTip", "空间复用搜索半径。推荐 8。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1).MaxValue(255)
				.Value_Lambda([this]() { return Settings.MaxHistoryLength; })
				.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type) { Settings.MaxHistoryLength = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("HistoryLengthTip", "时间复用最大历史长度。推荐 12。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(1.0f)
				.Value_Lambda([this]() { return Settings.NormalThreshold; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.NormalThreshold = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("NormalThresholdTip", "法线相似度阈值。推荐 0.35。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.001f).MaxValue(10.0f)
				.Value_Lambda([this]() { return Settings.DepthThreshold; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.DepthThreshold = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("DepthThresholdTip", "深度相似度阈值。推荐 0.06。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.01f).MaxValue(10.0f)
				.Value_Lambda([this]() { return Settings.RadianceClamp; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.RadianceClamp = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("RadianceClampTip", "限制二次辐射。推荐 3。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(20.0f)
				.Value_Lambda([this]() { return Settings.GIIntensity; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.GIIntensity = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("GIIntensityTip", "ReSTIR GI 强度。推荐 1.0；验证时可临时提高。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(10.0f)
				.Value_Lambda([this]() { return Settings.SyntheticConstantIntensity; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.SyntheticConstantIntensity = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("SyntheticIntensityTip", "Synthetic Constant 模式的辐射强度。推荐 0.5。"))
			]
			// --- Advanced Parameters ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 12, 0, 4)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.Text(LOCTEXT("AdvancedSection", "高级参数（通常保持默认）"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(1.0f).MaxValue(100000.0f)
				.Value_Lambda([this]() { return Settings.MaxRayDistance; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.MaxRayDistance = Value; FReSTIRGISettingsManager::RequestHistoryReset(); PushSettings(); })
				.ToolTipText(LOCTEXT("MaxRayDistanceTip", "二次射线最大距离。推荐 2500。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(50.0f)
				.Value_Lambda([this]() { return Settings.NormalBias; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.NormalBias = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("NormalBiasTip", "射线起点法线偏移。推荐 1.5。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(1.0f)
				.Value_Lambda([this]() { return Settings.DiffuseRayProbability; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.DiffuseRayProbability = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("DiffuseRayProbabilityTip", "漫反射 cosine 射线概率。推荐 1.0。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.02f).MaxValue(1.0f)
				.Value_Lambda([this]() { return Settings.SecondaryRoughnessClamp; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.SecondaryRoughnessClamp = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("SecondaryRoughnessClampTip", "限制低粗糙度表面不稳定性。推荐 0.5。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f).MaxValue(1.0f)
				.Value_Lambda([this]() { return Settings.BoilingFilterStrength; })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { Settings.BoilingFilterStrength = Value; PushSettings(); })
				.ToolTipText(LOCTEXT("BoilingFilterTip", "抑制权重跳变。推荐 0.75。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SSlider)
				.Value_Lambda([this]() { return Settings.CompareSplit; })
				.OnValueChanged_Lambda([this](float Value) { Settings.CompareSplit = Value; PushSettings(); })
			]
			// --- Checkboxes ---
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFinalMIS ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Settings.bFinalMIS = State == ECheckBoxState::Checked; PushSettings(); })
					[
						SNew(STextBlock).Text(LOCTEXT("MIS", "粗糙度 MIS"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFinalVisibility ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Settings.bFinalVisibility = State == ECheckBoxState::Checked; PushSettings(); })
					[
						SNew(STextBlock).Text(LOCTEXT("Visibility", "最终可见性"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bFreezeHistory ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Settings.bFreezeHistory = State == ECheckBoxState::Checked; PushSettings(); })
					[
						SNew(STextBlock).Text(LOCTEXT("FreezeHistory", "冻结历史"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bHalfResolution ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Settings.bHalfResolution = State == ECheckBoxState::Checked; FReSTIRGISettingsManager::RequestHistoryReset(); PushSettings(); })
					[
						SNew(STextBlock).Text(LOCTEXT("HalfResolution", "半分辨率"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Settings.bConservativeVisibility ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Settings.bConservativeVisibility = State == ECheckBoxState::Checked; PushSettings(); })
					[
						SNew(STextBlock).Text(LOCTEXT("ConservativeVisibility", "保守可见性"))
					]
				]
			]
			// --- Buttons ---
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetHistory", "重置历史"))
					.OnClicked_Lambda([]() { FReSTIRGISettingsManager::RequestHistoryReset(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetDefaults", "恢复推荐默认值"))
					.OnClicked_Lambda([this]()
					{
						const bool bWasEnabled = Settings.bEnabled;
						Settings = FReSTIRGISettings();
						Settings.bEnabled = bWasEnabled;
						FReSTIRGISettingsManager::RequestHistoryReset();
						PushSettings();
						return FReply::Handled();
					})
				]
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

TSharedRef<SWidget> SReSTIRGIDebugPanel::MakeLumenComboWidget(TSharedPtr<FEnumOption> Option) const
{
	return SNew(STextBlock).Text(Option.IsValid() ? FText::FromString(Option->Label) : FText::GetEmpty());
}

TSharedRef<SWidget> SReSTIRGIDebugPanel::MakeRadianceComboWidget(TSharedPtr<FEnumOption> Option) const
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

FText SReSTIRGIDebugPanel::GetLumenText() const
{
	for (const TSharedPtr<FEnumOption>& Option : LumenOptions)
	{
		if (Option.IsValid() && Option->Value == static_cast<uint8>(Settings.LumenCompareMode))
		{
			return FText::FromString(Option->Label);
		}
	}
	return LOCTEXT("UnknownLumenCompare", "Unknown");
}

FText SReSTIRGIDebugPanel::GetRadianceText() const
{
	for (const TSharedPtr<FEnumOption>& Option : RadianceOptions)
	{
		if (Option.IsValid() && Option->Value == static_cast<uint8>(Settings.RadianceMode))
		{
			return FText::FromString(Option->Label);
		}
	}
	return LOCTEXT("UnknownRadiance", "Unknown");
}

FText SReSTIRGIDebugPanel::GetStatsText() const
{
	const FReSTIRGIRuntimeStats Stats = FReSTIRGISettingsManager::GetStats();
	return FText::Format(
		LOCTEXT("Stats",
			"视图尺寸: {0} x {1}\n"
			"Reservoir 尺寸: {2} x {3}\n"
			"管线状态: {4}\n"
			"Reservoir 显存: {5} KB\n"
			"估算 rays/pixel: {6}\n"
			"历史有效: {7}\n"
			"ViewExtension 激活: {8}\n"
			"硬件光追可用: {9}\n"
			"表面有效率: {10}\n"
			"二次射线命中率: {11}\n"
			"Initial 有效率: {12}\n"
			"Temporal 有效率: {13}\n"
			"Spatial 有效率: {14}\n"
			"GPU 计时: 使用 ProfileGPU 或 stat GPU 查看"),
		FText::AsNumber(Stats.ViewSize.X),
		FText::AsNumber(Stats.ViewSize.Y),
		FText::AsNumber(Stats.ReservoirSize.X),
		FText::AsNumber(Stats.ReservoirSize.Y),
		FText::FromString(Stats.PipelineStatus),
		FText::AsNumber(Stats.ReservoirBytes / 1024),
		FText::AsNumber(Stats.EstimatedRaysPerPixel),
		BoolText(Stats.bHistoryValid),
		BoolText(Stats.bViewExtensionActive),
		BoolText(Stats.bRayTracingAvailable),
		RatioText(Stats.PrimaryValidRatio),
		RatioText(Stats.SecondaryHitRate),
		RatioText(Stats.InitialValidRatio),
		RatioText(Stats.TemporalValidRatio),
		RatioText(Stats.SpatialValidRatio));
}

#undef LOCTEXT_NAMESPACE
