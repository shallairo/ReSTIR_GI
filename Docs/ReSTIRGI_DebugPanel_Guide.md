# ReSTIR GI 调试面板使用指南

## 概述

ReSTIR GI 是一个面向学习与调试的**一跳（one-bounce）全局光照插件**。它通过 ReSTIR（Reservoir Spatio-Temporal Importance Resampling）算法对二次射线采样的间接光进行重采样，在 UE 5.8 的后处理管线中叠加间接光贡献。

**重要**：本插件不替代 Lumen，而是与 Lumen 共存或对比。最终的合成结果是 `UE Direct Lighting + ReSTIR GI`。

## 面板位置

`Window > ReSTIR GI Debug` 打开调试面板。

## 验证顺序（推荐）

### 第 1 步：确认基础设施

1. 启用 ReSTIR GI
2. 查看面板统计区：
   - **硬件光追可用** = Yes
   - **ViewExtension 激活** = Yes
   - **管线状态** = "ReSTIR GI pass 已调度..."
3. 如果**硬件光追可用** = No，请确认 DX12 + SM6 + 硬件光追已启用

### 第 2 步：验证表面与射线

1. 诊断视图 → **表面有效性**：大部分不透明表面应为蓝色，无效区域为暗红
2. 诊断视图 → **二次射线命中**：室内场景应显示明显绿色命中区域

### 第 3 步：用 Synthetic 模式验证算法闭环

1. 辐射模式 → **Synthetic Constant**（默认）
2. 算法模式 → **初始采样**
3. 诊断视图 → **仅看间接光（ToneMapped）**
4. 你应该看到橙色调的间接光，每帧有明显噪声
5. 切换到 **时间+空间复用**，噪声应明显降低

### 第 4 步：验证各阶段 Reservoir

1. 诊断视图 → **Initial Reservoir**：应显示逐帧噪声
2. 切换算法模式到 **时间复用**，诊断视图 → **Temporal Reservoir**：静止相机下应更平滑
3. 切换算法模式到 **时间+空间复用**，诊断视图 → **Spatial Reservoir**：应最平滑

### 第 5 步：切回真实辐射模式

1. 辐射模式 → **Screen Projected SceneColor**
2. 诊断视图 → **最终合成**
3. Lumen 对比 → **UE Direct + ReSTIR GI**（关闭 Lumen 观察独立贡献）
4. 调整 GI Intensity 观察间接光强度变化

## 参数说明

### 算法模式

| 模式 | 说明 |
|------|------|
| 关闭 | 不执行 ReSTIR GI |
| 初始采样 | 只使用 initial reservoir，每帧重新采样 |
| 时间复用 | 合并历史 reservoir（带 velocity 重投影） |
| 空间复用 | 合并邻域 reservoir |
| 时间+空间复用（推荐） | 先 temporal 再 spatial，效果最好 |

### 诊断视图

| 视图 | 说明 |
|------|------|
| 最终合成 | SceneColor + Indirect |
| 仅看间接光（ToneMapped） | 只显示 indirect，tone mapped |
| 仅看间接光（Linear） | 只显示 indirect，linear HDR |
| 仅看间接光（Tonemapped） | 只显示 indirect，tonemapped |
| 二次射线命中 | 绿色=命中，暗色=miss |
| 表面有效性 | 蓝色=有效 GBuffer，暗红=无效 |
| Initial Reservoir | 初始 reservoir 的 radiance × weightSum/M |
| Temporal Reservoir | temporal 阶段 reservoir |
| Spatial Reservoir | spatial 阶段 reservoir |
| Reservoir 权重 | weightSum/M 的热力图 |
| 历史年龄 | reservoir age / max history |
| 选中样本方向 | 选中样本方向编码为颜色 |
| TargetPdf | target PDF 热力图 |
| WeightSum/M | weightSum/M 热力图 |
| Indirect Raw | radiance × weightSum/M（不含 BRDF/Pdf） |

**注意**：诊断视图下不会触发 wipe/side-by-side，避免逻辑互相覆盖。

### 辐射模式

| 模式 | 说明 | 用途 |
|------|------|------|
| Synthetic Constant | 固定橙色辐射 | 验证算法闭环是否正确 |
| Normal Color | 命中法线编码为颜色 | 验证射线方向和命中 |
| Albedo Color | 使用 primary albedo 作为辐射 | 验证 reservoir 存储/回放 |
| Screen Projected SceneColor | 命中点屏幕投影采样 HDR | 真实场景近似 |

### Lumen 对比

| 模式 | 说明 |
|------|------|
| 叠加显示 | SceneColor + ReSTIR GI（Lumen 保持开启） |
| UE Direct + ReSTIR GI | 临时关闭 Lumen，只看直接光 + ReSTIR GI |
| 只看 UE/Lumen | 跳过 ReSTIR pass，只看 Lumen |
| 滑杆擦除对比 | 左侧 Composite，右侧 SceneColor |
| 左右分屏对比 | 左半 Composite，右半 SceneColor |

## 基础参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| Spatial Samples | 4 | 空间复用邻居数量 |
| Spatial Radius | 8 | 空间搜索半径 |
| Max History Length | 12 | 时间复用最大历史长度 |
| Normal Threshold | 0.35 | 法线相似度阈值 |
| Depth Threshold | 0.06 | 深度相似度阈值 |
| Radiance Clamp | 3.0 | 二次辐射上限 |
| GI Intensity | 1.0 | 间接光强度 |
| Synthetic Intensity | 0.5 | Synthetic Constant 模式辐射强度 |

## 高级参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| Max Ray Distance | 2500 | 二次射线最大距离 |
| Normal Bias | 1.5 | 射线起点法线偏移 |
| Diffuse Ray Probability | 1.0 | 漫反射射线概率 |
| Secondary Roughness Clamp | 0.5 | 粗糙度下限 |
| Boiling Filter Strength | 0.75 | 权重跳变抑制 |

## 控制台变量

| CVar | 说明 |
|------|------|
| `r.ReSTIRGI.Enabled` | -1=面板, 0=关, 1=开 |
| `r.ReSTIRGI.Mode` | -1=面板, 0-4 对应算法模式 |
| `r.ReSTIRGI.DebugView` | -1=面板, 0-14 对应诊断视图 |
| `r.ReSTIRGI.RadianceMode` | -1=面板, 0-3 对应辐射模式 |
| `r.ReSTIRGI.Intensity` | -1=面板, ≥0 覆盖强度 |
| `r.ReSTIRGI.MaxRayDistance` | -1=面板, ≥0 覆盖 |
| `r.ReSTIRGI.NormalBias` | -1=面板, ≥0 覆盖 |
| `r.ReSTIRGI.HalfResolution` | -1=面板, 0=全分辨率, 1=半分辨率 |
| `r.ReSTIRGI.ConservativeVisibility` | -1=面板, 0=关, 1=开 |
| `r.ReSTIRGI.BoilingFilterStrength` | -1=面板, ≥0 覆盖 |
| `r.ReSTIRGI.LumenCompareMode` | -1=面板, 0-4 对应对比模式 |

## 怎样判断算法真的生效

1. **Synthetic Constant 模式下，Indirect Only 必须有可见的橙色间接光**
2. **Initial Reservoir 视图必须呈现逐帧噪声**
3. **Temporal Reservoir 在静止相机下应比 Initial 更平滑**
4. **Spatial Reservoir 应比 Temporal 更平滑**
5. **切换算法模式时，诊断视图必须产生可观测差异**
6. **ProfileGPU 中能看到 4 个 pass：Initial Sample / Temporal / Spatial / Final Composite**

## 当前限制

- 二次命中点 radiance 使用屏幕投影 SceneColor 近似（非完整材质 hit shader）
- GPU 统计读回暂为占位，ratio 显示 N/A（需后续接入 RHIGPUBufferReadback）
- 不包含 NRD 去噪器
- 不包含完整多 bounce
- 半分辨率模式是 reservoir 半分辨率，不包含生产级上采样/降噪
