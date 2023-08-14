# AneRend

![Image](screenshot1.png)
_PBR Sponza example scene with global illumination (DDGI) and ray traced hard shadows turned on_

AneRend is a Vulkan renderer where I prototype state-of-the-art rendering techniques. Among those currently implemented in some form are:
+ Diffuse Dynamic Global Illumination (DDGI)
+ Ray-traced specular Global Illumination
+ Ray-traced hard shadows
+ Hierarchical Z occlussion culling (HiZ)
+ Bindless GPU-driven rendering 
+ Cook-Torrance BRDF PBR lighting
+ Deferred tiled rendering for many lights
+ Ghost of Tsushima-inspired procedural grass
+ Frame graph

The repo also contains (currently unused) stubs of surfel-based global illumination techniques.

## Global illumination based on irradiance field probes
The engine uses a realtime ray-traced irradiance probe approach for the diffuse global illumination. It is heavily inspired
by [Dynamic Diffuse Global Illumination](https://www.jcgt.org/published/0008/02/01/paper-lowres.pdf), but currently lacks the
statistical depth recording.

The camera is surrounded by a grid of irradiance probes. On a fixed time basis, defaulted at 30Hz, each probe samples the incoming
light by sending out a configurable amount of rays in a uniform spherical pattern. The pattern is a [fibonacci lattice](https://extremelearning.com.au/evenly-distributing-points-on-a-sphere/).
Either the rays hit nothing, in which case they return sky light. If not, they hit something in the scene, in which case 
they first have to send a shadow ray to check if they are in direct light. They then gather material data from the hit point
and shade this point using the same shading function as is used in the deferred
shading pass.

Since the same shading function is used as when shading the g-buffer, indirect light (global illumination) is also considered.
The indirect lighting calculation uses the probe data from previous frames. This effectively means that over time infinite bounce
global illumination is achieved.

The probe data is then compressed into an 8x8 octahehdron map for each probe, and put into a texture atlas. A convolution pass
takes the original N rays and sums the weighted radiances for each octahehdron direction to approximate the irradiance part of the
(diffuse) rendering equation. This convoluted version of each probe is then used when shading, essentially by multiplying
with the diffuse BRDF for each shade point.

In order to allow the GI to work for large scenes, the probes will move with the camera every time the camera crosses an integer
world space coordinate. A probe "translation" pass makes sure that probe data is appropriately copied when this happens, to 
ensure that probe data does not have to be recalculated.

There is an excellent talk by one of the paper authors here: [DDGI talk](https://www.youtube.com/watch?v=KufJBCTdn_o).

See the individual render passes below for more information, or feel free to ask about any details!

## Frame graph
The frame graph representation allows quicker development on the CPU-side. It also helps with a general overview of how a frame is constructed
as well as provides an abstraction layer where the engine can generalise details which would otherwise have to be manually handled, such as 
memory barriers between resource access. The implementation is based on [this talk by Yuriy O'Donnell](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)
from Frostbite EA.

Generally a render pass will look like this on the CPU-side:
```
  RenderPassRegisterInfo info{};
  info._name = "A render pass";

  {
    ResourceUsage usage{};
    usage._resourceName = "SomeTexture";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SomeOtherTexture";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "shader.spv";

  info._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("A render pass",
  [this](RenderExeParams exeParams) {

    // Bind pipeline
    vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

    vkCmdBindDescriptorSets(
      *exeParams.cmdBuffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      *exeParams.pipelineLayout,
      1, 1, &(*exeParams.descriptorSets)[0],
      0, nullptr);

    vkCmdDispatch(*exeParams.cmdBuffer, numX, numY, 1);
  });

```

The engine will automatically add appropriate resource barriers as well as things like debug timers for each registered render pass. 
When the execution of the pass begins, all relevant resources are accessible via the exeParams, e.g. the descriptor sets. Since
the engine uses a "bindless" approach, a single buffer for geometry vertices and indices is used and bound by the engine before all the
render passes execute. Additional bindless descriptors include a material buffer, a scene UBO, a mesh buffer amongst others. This way
only the specific descriptors (and the pipeline) used by the render pass are bound, reducing overhead.

Below is a list of the (current) render passes and some discussions on key parts.

## HiZ
Since the engine utilises a completely GPU-driven draw call generation, some form of culling of renderables is required. This pass generates
the hierarchical depth mips used for occlusion culling.

The idea is to use the previous frame's depth buffer and generate a mip chain from it. When culling each renderable, the culling shader
looks up a level where each pixel roughly corresponds to the current renderables bounding volume's size. 

The trick is to use a special sampler for sampling the depth buffer when generating the mips, that always chooses the maximum value of
the region it is sampling from. This way a conservative depth mip is achieved.

Here is an example of how this mip may look like (rendered scene, 64x64, 32x32 and 4x4 mips shown):

| ![Image](screenshots/hiz/hiz_source.png) |
|:--:|
| _Source for the hiz mips_ |

| ![Image](screenshots/hiz/hiz64.png) |
|:--:|
| _64x64 'max-sampled' mip_ |

|![Image](screenshots/hiz/hiz32.png)|
|:--:|
|_32x32 'max-sampled' mip_|

|![Image](screenshots/hiz/hiz4.png)|
|:--:|
|_4x4 'max-sampled' mip_|

References: 
https://interplayoflight.wordpress.com/2017/11/15/experiments-in-gpu-based-occlusion-culling/

## Cull
The culling pass actually generates draw calls directly on the GPU. For rendering the geometry (non-procedurally generated meshes)
, the CPU only issues a single indirect draw call, referencing a buffer filled in by this render pass.

A single compute shader is run, receiving a list of all potential renderables in the scene. In addition to this, metadata
for the renderables is accessible, most notably bounding volumes. The metadata in combination with the HiZ mip and camera parameters
allows the shader to do both frustum culling and occlusion culling. Since each renderable culling calculation is independent,
this quickly yields near-optimal draw calls only for meshes that are currently visible.

References: 
https://vkguide.dev/docs/gpudriven/gpu_driven_engines/
https://vkguide.dev/docs/gpudriven/compute_culling/

## IrradianceProbeTrans
This pass relates to the diffuse global illumination. 

Each frame, the camera is checked for world coordinate integer crossings. I.e. moving from 0 to 1 or 54 to 53 etc. in any axis.
If the camera has crossed such a boundary, the probe grid has to be moved accordingly in order to keep up with the camera.
However, it is vital to ensure that the probe data is not lost when this happens since the multi-bounce GI depends on 
at least one previous frame's probe data to work properly. 

If the probes were abstracted in a buffer-oriented way, this would not really be an issue. But since they are stored in a
texture atlas, the texture atlas has to be "translated" so that the new probes on the edge of the probe grid have somewhere
to store their data.

This pass achieves this by issuing a single `vkCmdCopyImage` with appropriate regions. The screenshots illustrate this:

|![Image](screenshots/ddgi/pre_translate.png)|
|:--:|
|_Irradiance probe atlas before translating_|

|![Image](screenshots/ddgi/post_translate.png)|
|:--:|
|_Irradiance probe atlas after translating, note how all probes have translated one step to the left_|

This translation allows the multi-bounce GI to still work. The edges where new probes will be filled in may flicker and result
in artifacts. This can be solved by making the probe grid bigger than the far plane or simply fading GI out over distance.

## IrradianceProbeRT
This pass relates to the diffuse global illumination. 

## IrradianceProbeConvolve

## Geometry

## Grass

## ShadowRT

## SpecularGIRT

## SpecularGIMipGen

## SSAO and SSAOBlur

## FXAA

## DebugBoundingSpheres and DebugView

## UI

## Present
