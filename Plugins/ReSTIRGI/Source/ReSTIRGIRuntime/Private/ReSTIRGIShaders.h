#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "SceneView.h"
#include "RenderGraphResources.h"
#include "SceneTexturesConfig.h"
#include "ShaderCompilerCore.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"

struct FReSTIRGISurface
{
	FVector4f WorldPositionAndDepth = FVector4f::Zero();
	FVector4f NormalAndRoughness = FVector4f(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4f BaseColorAndSpecular = FVector4f(1.0f, 1.0f, 1.0f, 0.5f);
	FVector4f Misc = FVector4f::Zero();
};

struct FReSTIRGIPackedReservoir
{
	FVector4f SamplePositionAndWeight = FVector4f::Zero();
	FVector4f SampleNormalAndPdf = FVector4f::Zero();
	FVector4f RadianceAndM = FVector4f::Zero();
	FUintVector4 Misc = FUintVector4(0, 0, 0, 0);
};

struct FReSTIRGIStats
{
	FUintVector4 PrimaryValidAndHitAndInitValid = FUintVector4(0, 0, 0, 0);
	FUintVector4 TemporalSpatialValidAndCount = FUintVector4(0, 0, 0, 0);
	FUintVector4 Pad1 = FUintVector4(0, 0, 0, 0);
	FUintVector4 Pad2 = FUintVector4(0, 0, 0, 0);
};

BEGIN_SHADER_PARAMETER_STRUCT(FReSTIRGICommonShaderParameters, )
	SHADER_PARAMETER(FIntPoint, ViewRectMin)
	SHADER_PARAMETER(FIntPoint, ViewRectSize)
	SHADER_PARAMETER(FIntPoint, SceneColorTextureSize)
	SHADER_PARAMETER(FIntPoint, ReservoirSize)
	SHADER_PARAMETER(uint32, FrameIndex)
	SHADER_PARAMETER(uint32, ResamplingMode)
	SHADER_PARAMETER(uint32, DebugView)
	SHADER_PARAMETER(uint32, RadianceMode)
	SHADER_PARAMETER(uint32, SpatialSampleCount)
	SHADER_PARAMETER(uint32, MaxHistoryLength)
	SHADER_PARAMETER(float, SpatialRadius)
	SHADER_PARAMETER(float, NormalThreshold)
	SHADER_PARAMETER(float, DepthThreshold)
	SHADER_PARAMETER(float, RoughnessMISClamp)
	SHADER_PARAMETER(float, RadianceClamp)
	SHADER_PARAMETER(float, GIIntensity)
	SHADER_PARAMETER(float, CompareSplit)
	SHADER_PARAMETER(float, MaxRayDistance)
	SHADER_PARAMETER(float, NormalBias)
	SHADER_PARAMETER(float, DiffuseRayProbability)
	SHADER_PARAMETER(float, SecondaryRoughnessClamp)
	SHADER_PARAMETER(float, BoilingFilterStrength)
	SHADER_PARAMETER(float, SyntheticConstantIntensity)
	SHADER_PARAMETER(uint32, bFinalMIS)
	SHADER_PARAMETER(uint32, bFinalVisibility)
	SHADER_PARAMETER(uint32, bHistoryValid)
	SHADER_PARAMETER(uint32, bRayTracingAvailable)
	SHADER_PARAMETER(uint32, bConservativeVisibility)
	SHADER_PARAMETER(uint32, LumenCompareMode)
END_SHADER_PARAMETER_STRUCT()

class FReSTIRGIInitialSampleCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGIInitialSampleCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGIInitialSampleCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsInlineRayTracing(Parameters.Platform) && FDataDrivenShaderPlatformInfo::GetSupportsInlineRayTracing(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_Wave32);
		OutEnvironment.CompilerFlags.Add(CFLAG_InlineRayTracing);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGISurface>, PrimarySurfaceBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGISurface>, SecondarySurfaceBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, InitialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIStats>, StatsBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGITemporalResamplingCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGITemporalResamplingCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGITemporalResamplingCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsInlineRayTracing(Parameters.Platform) && FDataDrivenShaderPlatformInfo::GetSupportsInlineRayTracing(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_Wave32);
		OutEnvironment.CompilerFlags.Add(CFLAG_InlineRayTracing);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGISurface>, PrimarySurfaceBufferSRV)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGISurface>, HistoryPrimarySurfaceBuffer)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, InitialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, HistoryReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, TemporalReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIStats>, StatsBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGISpatialResamplingCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGISpatialResamplingCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGISpatialResamplingCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsInlineRayTracing(Parameters.Platform) && FDataDrivenShaderPlatformInfo::GetSupportsInlineRayTracing(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_Wave32);
		OutEnvironment.CompilerFlags.Add(CFLAG_InlineRayTracing);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGISurface>, PrimarySurfaceBufferSRV)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, NeighborOffsetBuffer)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, SourceReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, SpatialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIStats>, StatsBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FReSTIRGIFinalCompositeCS final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FReSTIRGIFinalCompositeCS);
	SHADER_USE_PARAMETER_STRUCT(FReSTIRGIFinalCompositeCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsInlineRayTracing(Parameters.Platform) && FDataDrivenShaderPlatformInfo::GetSupportsInlineRayTracing(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_Wave32);
		OutEnvironment.CompilerFlags.Add(CFLAG_InlineRayTracing);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FReSTIRGICommonShaderParameters, Common)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGISurface>, PrimarySurfaceBufferSRV)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGISurface>, SecondarySurfaceBufferSRV)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, FinalReservoir)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, DebugInitialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, DebugTemporalReservoir)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FReSTIRGIPackedReservoir>, DebugSpatialReservoir)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGISurface>, HistoryPrimarySurfaceOutput)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIPackedReservoir>, HistoryReservoirOutput)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FReSTIRGIStats>, StatsBuffer)
	END_SHADER_PARAMETER_STRUCT()
};
