# UE 5.8 ReSTIR GI 当前实现与后续计划

## 当前目标

`Plugins/ReSTIRGI` 当前实现的是面向学习和调试的一跳 ReSTIR GI 闭环。它保留纯 C++、RDG、Global Shader、Slate UI 工作流，不使用蓝图。

本阶段优先保证三件事：

- 算法路径真实存在：primary surface、secondary ray、reservoir、temporal/spatial resampling、final shading。
- UI 逻辑清晰：算法模式、诊断视图、Lumen 对比分离。
- 效果可验证：提供表面有效性、二次射线命中、仅看间接光、reservoir 权重和历史年龄视图。

## 当前实现

- Runtime 使用 `FReSTIRGIViewExtension` 在 Tonemap 前接入 HDR post-process 链，避免多次叠加。
- 使用 `UE::FXRenderingUtils::RayTracing::GetRayTracingSceneViewRDG` 获取 UE 5.8 公共 TLAS SRV。
- `只看 ReSTIR GI` 模式下，ViewExtension 会将当前 view 的 `DynamicGlobalIlluminationMethod` 临时设为 `Plugin`，并通过 UE GI plugin ray tracing delegate 声明需要 RayTracingScene，避免关闭 Lumen 后 TLAS 不再构建。
- Initial pass 从 GBuffer 重建 primary surface，并用 inline ray tracing 发射一条二次射线。
- 二次射线方向默认使用 diffuse cosine sampling，后续可逐步恢复 diffuse/glossy 更完整的 MIS。
- Secondary radiance 当前采用“命中点屏幕投影采样 HDR SceneColor + sky fallback”的近似方案。
- Temporal pass 使用 depth、normal、history age 过滤并合并 history reservoir。
- Spatial pass 使用 neighbor offset buffer、depth/normal 阈值和 targetPdf 合并邻域 reservoir。
- Final pass 使用 primary diffuse BRDF、reservoir radiance、稳定权重、radiance clamp、GI intensity 输出 composite。
- Slate 面板已收敛为算法模式、诊断视图、Lumen 对比、基础参数、高级参数和恢复默认值。

## 默认值策略

默认值不追求“看起来最强”，而是追求“可验证、不过曝、稳定”：

- `GIIntensity=0.35`
- `RadianceClamp=3`
- `SpatialSampleCount=4`
- `SpatialRadius=8`
- `MaxHistoryLength=12`
- `MaxRayDistance=2500`
- `NormalBias=1.5`
- `BoilingFilterStrength=0.75`
- `bHalfResolution=true`
- `bFinalVisibility=false`

如果需要演示效果，可以临时提高 `GIIntensity`，但验证完成后应回到推荐值。

## UI 设计约束

- `算法模式` 只控制 reservoir 阶段：关闭、初始采样、时间复用、空间复用、时间+空间复用。
- `诊断视图` 只控制算法数据可视化：最终合成、仅看间接光、二次射线命中、Reservoir 权重、历史年龄、表面有效性。
- `Lumen 对比` 只控制最终画面对比：叠加显示、只看 ReSTIR GI、只看 UE/Lumen、滑杆擦除、左右分屏。
- 不再在 Debug View 中重复放置 Wipe/Side-by-side，避免和 Lumen Compare 逻辑冲突。

## 当前限制

- 尚未实现完整 UE 材质 hit shader，二次命中点 radiance 不是完整材质照明。
- 尚未迁移 RTXDI DI、ReGIR、mesh light、NRD denoiser。
- temporal reprojection 仍以 reservoir 网格和 surface 过滤为主，尚未完整使用 velocity 历史像素重投影。
- GPU timing 不再在 UI 中伪造 0 ms；真实时间通过 `ProfileGPU` 或 `stat GPU` 查看 `ReSTIRGI` RDG scope。
- 当前方案用于学习和调试，不替代 Lumen 生产级 GI。

## 下一步建议

1. 接入 velocity reprojection，让 history reservoir 按上一帧像素位置读取。
2. 增加 GPU reduction/readback，统计真实 valid reservoir ratio。
3. 增加轻量 secondary hit lighting，用 emissive、skylight、directional light 估计替代纯 screen-projected radiance。
4. 增加 bilateral/SVGF 风格滤波，降低仅一条 secondary ray 的噪声。
5. 若追求更接近 Lumen，再评估更深的 Deferred Renderer 插入点。
