---
name: ReSTIR_GI_Algorithm_Fix
overview: 修复ReSTIR GI插件的两个严重Bug：(1)滤波参数修改无效 (2)PIE启动后黑屏。通过对比RTXDI标准实现，定位并修复算法逻辑差异、数学错误、渲染Pass依赖和资源生命周期问题。
todos:
  - id: fix-combine-math
    content: 修复 CombineReservoirs 数学：从 ReservoirCandidateWeightForSurface 移除 * CandidateCount 乘法，使 weightSum 正确反映 targetPdf/samplePdf 比率
    status: completed
  - id: fix-sample-pdf
    content: 修复 SampleSecondaryDirection PDF：将输出 PDF 改为 MIS 合并的 DiffuseRayProbability*DiffusePdf + (1-DiffuseRayProbability)*GlossyPdf
    status: completed
  - id: fix-pie-output
    content: 修复 PIE 黑屏：在 FinalComposite pass 前添加 CopyTexture pass 将 SceneColor 复制到 OutputTexture，确保全像素覆盖
    status: completed
  - id: fix-pie-lumen
    content: 修复 PIE Lumen 状态污染：在 BeginRenderViewFamily 中检测 PIE 模式并重置 static bStoredPreviousValues
    status: completed
  - id: fix-pie-history
    content: 修复 PIE 历史缓冲区：在 AddReSTIRGIPass_RenderThread 中添加 PIE 启动时的强制历史重置逻辑
    status: completed
  - id: verify-compilation
    content: 编译验证：确保所有修改通过 myGIEditor Win64 Development 编译，shader 无错误
    status: completed
    dependencies:
      - fix-combine-math
      - fix-sample-pdf
      - fix-pie-output
      - fix-pie-lumen
      - fix-pie-history
---

## 用户需求

修复 ReSTIR GI 插件的两个关键问题：

1. 修改时空滤波参数（SpatialSampleCount、SpatialRadius、MaxHistoryLength、NormalThreshold、DepthThreshold、BoilingFilterStrength）均无任何效果变化
2. 启动 Play（PIE）后画面直接黑屏

## 产品概述

ReSTIR GI 是面向 UE 5.8 的一跳 GI 学习/调试插件，通过四次 compute pass 实现 initial sample → temporal resampling → spatial resampling → final composite 的完整 ReSTIR GI 管线。修复后应恢复滤波参数的可观测响应，并消除 PIE 黑屏。

## 核心修复目标

- 修正 Reservoir 组合数学中的 M 双重计数错误，恢复参数响应
- 修复 SampleSecondaryDirection 的 PDF 计算，使初始采样的概率归一化正确
- 确保 OutputTexture 全像素覆盖，消除 PIE 黑屏
- 修复 PIE 跨会话的 Lumen 状态污染和历史缓冲区尺寸不匹配

## 技术栈

- UE 5.8 + RDG (Render Dependency Graph)
- HLSL Compute Shader (Inline Ray Tracing, Wave32)
- FSceneViewExtensionBase (AfterTonemap 回调)
- Slate UI (调试面板)

## 已识别的关键差异与根因

### 差异 1：CombineReservoirs 数学错误（参数无效果的核心根因）

**当前代码**（ReSTIRGI.usf:334-348）：

```
float ReservoirCandidateWeightForSurface(...) {
    return max(CurrentTargetPdf / StoredSamplePdf, 0.0) * CandidateCount; // ← 乘了 M
}
// CombineReservoirs 中:
OutR.RadianceAndM.w = A.RadianceAndM.w + B.RadianceAndM.w; // ← 又累加了 M
```

**RTXDI 标准**：候选权重应为 `targetPdf/samplePdf`（重新评估的比率），不乘 M。M 应独立累加用于 1/M 归一化。
**后果**：经过多次 combine 后 `weightSum/M` 变成 `(w_i*M_i之和)/(M_i之和)`，丧失了正确的概率归一化含义，使参数调整无法产生可观测效果。

### 差异 2：SampleSecondaryDirection PDF 未合并概率

**当前代码**（ReSTIRGI.usf:214-241）：当选择 glossy 方向时，`RayPdf = GlossyPdf`，当选择 diffuse 时 `RayPdf = DiffusePdf`。
**RTXDI 标准**（BrdfRayTracing.hlsl:124-131）：`overall_PDF = lerp(diffuseLobe_PDF, specularLobe_PDF, specular_PDF)`。
**后果**：初始 reservoir 的 `weight = targetPdf/samplePdf` 基于错误的 PDF，导致初始采样权重偏移。

### 差异 3：缺少 FinalizeGIResampling 归一化

**当前代码**：FinalComposite 中手动 `weightSum/M`（usf:798-799）。
**RTXDI 标准**：调用 `RTXDI_FinalizeGIResampling` 进行 MIS-like 归一化（考虑源 reservoir 的 targetPdf）。
**后果**：重采样后的 reservoir 权重未经过适当的 bias correction 归一化。

### 差异 4：PIE 黑屏根因

**根因 4a**：OutputTexture 为新建 RDG 纹理（ViewExtension.cpp:525-527），FinalCompositeCS 仅写入 ViewRect 内像素（usf:776 IsInsideView），ViewRect 外为未初始化的垃圾数据。
**根因 4b**：`ApplyLumenCompareMode` 使用 `static bool bStoredPreviousValues`（ViewExtension.cpp:196），PIE 跨会话时不重置，导致 Lumen CVar 状态异常。
**根因 4c**：PIE 启动时 bHistoryValid 可能为 true，但历史缓冲区尺寸与 PIE 视口不匹配，RegisterExternalBuffer 可能失败。

## 修复方案

### 修复 1：修正 CombineReservoirs 数学

- 从 `ReservoirCandidateWeightForSurface` 移除 `* CandidateCount`
- 修改后 `return max(CurrentTargetPdf / StoredSamplePdf, 0.0);`
- 这使得 weightSum = Σ(targetPdf_j/samplePdf_j)，M = Σ(M_j)，FinalComposite 中 `weightSum/M` 成为正确的平均值

### 修复 2：修正 SampleSecondaryDirection PDF

- 将 GlennPdf 修改为 MIS 合并的 PDF
- `RayPdf = max(DiffuseRayProbability * DiffusePdf + (1.0 - DiffuseRayProbability) * GlossyPdf, 1e-4);`
- 当 DiffuseRayProbability=1.0 时结果不变，维持向后兼容

### 修复 3：PIE OutputTexture 全覆盖

- 在 FinalComposite pass 前添加 CopyTexture pass，将 SceneColor 复制到 OutputTexture
- 使用 `AddCopyTexturePass` 确保所有像素（包括 ViewRect 外）都有有效值
- FinalCompositeCS 仅覆盖 ViewRect 内像素，其余区域保持 SceneColor 原值

### 修复 4：PIE Lumen 状态重启

- 在 `BeginRenderViewFamily` 中检测 PIE 模式（通过 `GIsEditor && GIsPlayInEditorWorld`），重置 `bStoredPreviousValues`
- 确保 PIE 启动时 Lumen CVar 恢复到正确的原始值

### 修复 5：PIE 历史状态管理

- 在 `AddReSTIRGIPass_RenderThread` 的 buffer 注册前添加 `TRefCountPtr` 有效性联合检查
- 若尺寸不匹配时，确保 `RegisterExternalBuffer` 不会因 null buffer 导致问题