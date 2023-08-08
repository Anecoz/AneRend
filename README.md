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

Below is a list of the (current) render passes and some discussions on key parts.

## HiZ

## Cull

## IrradianceProbeRT

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
