#pragma once

#include "CoreMinimal.h"

enum class EReSTIRGIResamplingMode : uint8
{
	Off = 0,
	InitialOnly,
	Temporal,
	Spatial,
	TemporalAndSpatial
};

enum class EReSTIRGIDebugView : uint8
{
	Composite = 0,
	InitialOnly,
	Temporal,
	Spatial,
	ReservoirWeight,
	SampleAge,
	Heatmap,
	WipeCompare,
	SideBySide
};

struct FReSTIRGISettings
{
	bool bEnabled = false;
	bool bFinalMIS = true;
	bool bFinalVisibility = false;
	bool bFreezeHistory = false;
	bool bResetHistory = false;

	EReSTIRGIResamplingMode ResamplingMode = EReSTIRGIResamplingMode::TemporalAndSpatial;
	EReSTIRGIDebugView DebugView = EReSTIRGIDebugView::Composite;

	int32 SpatialSampleCount = 4;
	int32 MaxHistoryLength = 20;
	float SpatialRadius = 16.0f;
	float NormalThreshold = 0.5f;
	float DepthThreshold = 0.1f;
	float RoughnessMISClamp = 0.3f;
	float RadianceClamp = 10.0f;
	float CompareSplit = 0.5f;
};

struct FReSTIRGIRuntimeStats
{
	FIntPoint ViewSize = FIntPoint::ZeroValue;
	uint64 ReservoirBytes = 0;
	int32 EstimatedRaysPerPixel = 0;
	bool bViewExtensionActive = false;
	bool bHistoryValid = false;
};

class RESTIRGIRUNTIME_API FReSTIRGISettingsManager
{
public:
	static FReSTIRGISettings GetSettings();
	static FReSTIRGIRuntimeStats GetStats();
	static void UpdateSettings(const FReSTIRGISettings& InSettings);
	static void RequestHistoryReset();
	static void UpdateStats(const FReSTIRGIRuntimeStats& InStats);

private:
	static FCriticalSection CriticalSection;
	static FReSTIRGISettings Settings;
	static FReSTIRGIRuntimeStats Stats;
};
