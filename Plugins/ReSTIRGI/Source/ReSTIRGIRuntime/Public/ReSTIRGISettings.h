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
	IndirectOnly,
	IndirectOnlyLinear,
	IndirectOnlyTonemapped,
	HitMask,
	SurfaceValidity,
	InitialReservoir,
	TemporalReservoir,
	SpatialReservoir,
	ReservoirWeight,
	SampleAge,
	SelectedSample,
	TargetPdf,
	WeightSumM,
	IndirectRaw
};

enum class EReSTIRGILumenCompareMode : uint8
{
	Overlay = 0,
	ReSTIROnly,
	LumenOnly,
	Wipe,
	SideBySide
};

enum class EReSTIRGIRadianceMode : uint8
{
	ScreenProjected = 0,
	SyntheticConstant,
	NormalColor,
	AlbedoColor
};

struct FReSTIRGISettings
{
	bool bEnabled = false;
	bool bFinalMIS = true;
	bool bFinalVisibility = false;
	bool bFreezeHistory = false;
	bool bResetHistory = false;
	bool bHalfResolution = true;
	bool bConservativeVisibility = false;

	EReSTIRGIResamplingMode ResamplingMode = EReSTIRGIResamplingMode::TemporalAndSpatial;
	EReSTIRGIDebugView DebugView = EReSTIRGIDebugView::Composite;
	EReSTIRGILumenCompareMode LumenCompareMode = EReSTIRGILumenCompareMode::Overlay;
	EReSTIRGIRadianceMode RadianceMode = EReSTIRGIRadianceMode::SyntheticConstant;

	int32 SpatialSampleCount = 4;
	int32 MaxHistoryLength = 12;
	float SpatialRadius = 8.0f;
	float NormalThreshold = 0.35f;
	float DepthThreshold = 0.06f;
	float RoughnessMISClamp = 0.25f;
	float RadianceClamp = 3.0f;
	float GIIntensity = 1.0f;
	float CompareSplit = 0.5f;
	float MaxRayDistance = 2500.0f;
	float NormalBias = 1.5f;
	float DiffuseRayProbability = 1.0f;
	float SecondaryRoughnessClamp = 0.5f;
	float BoilingFilterStrength = 0.75f;
	float SyntheticConstantIntensity = 0.5f;
};

struct FReSTIRGIRuntimeStats
{
	FIntPoint ViewSize = FIntPoint::ZeroValue;
	FIntPoint ReservoirSize = FIntPoint::ZeroValue;
	FString PipelineStatus;
	uint64 ReservoirBytes = 0;
	int32 EstimatedRaysPerPixel = 0;
	bool bViewExtensionActive = false;
	bool bHistoryValid = false;
	bool bRayTracingAvailable = false;
	float PrimaryValidRatio = -1.0f;
	float SecondaryHitRate = -1.0f;
	float InitialValidRatio = -1.0f;
	float TemporalValidRatio = -1.0f;
	float SpatialValidRatio = -1.0f;
	float AvgWeight = -1.0f;
	float AvgRadiance = -1.0f;
	float AvgIndirectLuminance = -1.0f;
	float InitialMs = 0.0f;
	float TemporalMs = 0.0f;
	float SpatialMs = 0.0f;
	float FinalMs = 0.0f;
	EReSTIRGILumenCompareMode CurrentLumenMode = EReSTIRGILumenCompareMode::Overlay;
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
