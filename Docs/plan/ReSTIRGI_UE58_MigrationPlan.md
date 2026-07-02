# UE 5.8 ReSTIR GI Migration Plan

## Goal

This project implements a medium-scope ReSTIR GI learning and debugging path for UE 5.8. The first implementation lives in `Plugins/ReSTIRGI` and uses C++ modules, UE global shaders, RDG passes, and a pure Slate editor panel. Blueprints are intentionally not part of the workflow.

The current implementation establishes the integration spine:

- `ReSTIRGIRuntime` registers shader directory mappings, owns the SceneViewExtension, snapshots settings, and dispatches RDG compute passes.
- `ReSTIRGIEditor` adds `Window > ReSTIR GI Debug` with controls for mode, debug view, reservoir history, spatial sampling, MIS, visibility, and realtime comparison.
- `/Plugin/ReSTIRGI/ReSTIRGI.usf` contains the first shader-side reservoir representation and four-pass ReSTIR GI flow.

## Algorithm Scope

Version 0.1 targets a compact but useful ReSTIR GI loop:

1. Initial reservoir generation.
2. Temporal reservoir reuse with ping-ponged history.
3. Spatial neighbor resampling.
4. Final composite and debug visualization.

The current shader uses SceneColor-derived proxy radiance so the plugin can be wired and debugged before the hardware ray tracing path is inserted. The next algorithm step is to replace `InitialSampleCS` with a ray tracing or ray query path that traces a secondary surface, shades it, and writes actual secondary-surface position, normal, radiance, pdf, throughput, and flags.

## UE Rendering Integration

Runtime integration is designed around UE renderer-native systems:

- `FReSTIRGIViewExtension` subscribes to the post-processing chain and receives `SceneColor` as the baseline input.
- RDG buffers are allocated per view size for initial, temporal, spatial, and history reservoirs.
- The history reservoir is extracted through RDG and re-registered on the next frame.
- Resolution changes, explicit reset, and freeze toggles control history lifetime.
- Final output is returned as an `FScreenPassTexture`, enabling composite, wipe compare, side-by-side, and heatmap debug modes.

The later hardware RT phase should add:

- Ray tracing feature checks and user-facing disabled state.
- Scene depth, normals, roughness, velocity, and view uniforms in shader parameters.
- A real secondary-surface buffer with material approximation and pdf.
- Optional final visibility ray support.

## C++ and Shader Data Contract

The shared setting model is `FReSTIRGISettings`:

- Enable, resampling mode, debug view.
- Spatial sample count, radius, normal/depth thresholds.
- Max history, MIS clamp, radiance clamp, compare split.
- MIS, final visibility, freeze history, reset history.

The shader reservoir contract is `FReSTIRGIPackedReservoir`:

- `SamplePositionAndWeight`: xyz sample position, w reservoir weight.
- `SampleNormalAndPdf`: xyz sample normal, w sample pdf.
- `RadianceAndM`: rgb radiance, w sample count.
- `Misc`: age and future debug flags.

This layout intentionally mirrors the RTXDI ReSTIR GI reservoir concept while staying local to UE shader code and avoiding a hard dependency on the currently incomplete `RTXDI-main/Libraries/Rtxdi` submodule.

## Editor Debug UI

The debug UI is implemented in C++ Slate:

- Menu entry: `Window > ReSTIR GI Debug`.
- Enable checkbox and mode selectors.
- Controls for spatial samples, radius, max history, normal threshold, radiance clamp, and compare split.
- Toggles for MIS, final visibility, and freeze history.
- Reset history button.
- Runtime stats for view size, reservoir memory, estimated rays per pixel, history validity, and ViewExtension activity.

The UI only mutates C++ settings. Rendering comparison is performed by the RDG final composite pass. Console overrides are also available for quick validation: `r.ReSTIRGI.Enabled`, `r.ReSTIRGI.Mode`, and `r.ReSTIRGI.DebugView`; a value of `-1` defers to the Slate panel.

## Next Engineering Steps

1. Compile the plugin against the local UE 5.8 source build and fix API signature differences if the engine branch has changed post-processing extension types.
2. Replace `InitialSampleCS` proxy radiance with a ray tracing or ray query shader that writes true secondary-surface reservoirs.
3. Bind GBuffer inputs for depth, normals, roughness, and velocity; then upgrade temporal reprojection from same-pixel history to motion-vector reprojection.
4. Add conservative visibility and final visibility rays behind toggles.
5. Add GPU timing queries or RDG event statistics to fill the timing section of the editor panel.

## Acceptance Checks

- Project loads with the `ReSTIRGI` plugin enabled.
- `Window > ReSTIR GI Debug` opens without Blueprints.
- Enabling ReSTIR GI changes the viewport through the final composite pass.
- `Initial Only`, `Temporal`, `Spatial`, `Temporal + Spatial`, `Heatmap`, `Wipe Compare`, and `Side By Side` modes produce distinct output.
- History reset and resolution changes do not crash.
- No RTXDI proprietary sample source is copied verbatim into project code.
