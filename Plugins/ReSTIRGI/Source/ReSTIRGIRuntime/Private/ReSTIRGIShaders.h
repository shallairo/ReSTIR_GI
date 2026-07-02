#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderGraphResources.h"
#include "ShaderParameterStruct.h"

struct FReSTIRGIPackedReservoir
{
	FVector4f SamplePositionAndWeight = FVector4f::Zero();
	FVector4f SampleNormalAndPdf = FVector4f::Zero();
	FVector4f RadianceAndM = FVector4f::Zero();
	FUintVector4 Misc = FUintVector4(0, 0, 0, 0);
};

BEGIN_SHADER_PARAMETER_STRUCT(FReSTIRGICommonShaderParameters, )
	SHADER_PARAMETER(FIntPoint, ViewSize)
	SHADER_PARAMETER(uint32, FrameIndex)
	SHADER_PARAMETER(uint32, ResamplingMode)
	SHADER_PARAMETER(uint32, DebugView)
	SHADER_PARAMETER(uint32, SpatialSampleCount)
	SHADER_PARAMETER(uint32, MaxHistoryLength)
	SHADER_PARAMETER(float, SpatialRadius)
	SHADER_PARAMETER(float, NormalThreshold)
	SHADER_PARAMETER(float, DepthThreshold)
	SHADER_PARAMETER(float, RoughnessMISClamp)
	SHADER_PARAMETER(float, RadianceClamp)
	SHADER_PARAMETER(float, CompareSplit)
	SHADER_PARAMETER(uint32, bFinalMIS)
	SHADER_PARAMETER(uint32, bFinalVisibility)
	SHADER_PARAMETER(uint32, bHistoryValid)
END_SHADER_PARAMETER_STRUCT()

class FReSTIRGIInitialSampleCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGIInitialSampleCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGIInitialSampleCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, InitialReservoir)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGITemporalResamplingCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGITemporalResamplingCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGITemporalResamplingCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, InitialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, HistoryReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, TemporalReservoir)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGISpatialResamplingCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGISpatialResamplingCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGISpatialResamplingCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, SourceReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, SpatialReservoir)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGIFinalCompositeCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGIFinalCompositeCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGIFinalCompositeCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, FinalReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, HistoryReservoirOutput)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};
