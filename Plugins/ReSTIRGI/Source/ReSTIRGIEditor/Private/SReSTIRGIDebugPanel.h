#pragma once

#include "CoreMinimal.h"
#include "ReSTIRGISettings.h"
#include "Widgets/SCompoundWidget.h"

class SReSTIRGIDebugPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReSTIRGIDebugPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FEnumOption
	{
		FEnumOption(const FString& InLabel, const uint8 InValue)
			: Label(InLabel)
			, Value(InValue)
		{
		}

		FString Label;
		uint8 Value;
	};

	void PushSettings() const;
	TSharedRef<SWidget> MakeModeComboWidget(TSharedPtr<FEnumOption> Option) const;
	TSharedRef<SWidget> MakeDebugComboWidget(TSharedPtr<FEnumOption> Option) const;
	FText GetModeText() const;
	FText GetDebugText() const;
	FText GetStatsText() const;

	FReSTIRGISettings Settings;
	TArray<TSharedPtr<FEnumOption>> ModeOptions;
	TArray<TSharedPtr<FEnumOption>> DebugOptions;
};
