%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard.vert -o %2/standard_vert.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard_shadow.vert -o %2/standard_shadow_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard_shadow_point.vert -o %2/standard_shadow_point_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard_shadow.frag -o %2/standard_shadow_frag.spv

REM %VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard_instanced.vert -o %2/standard_instanced_vert.spv
REM %VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard_instanced_shadow.vert -o %2/standard_instanced_shadow_vert.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/standard.frag -o %2/standard_frag.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/fullscreen.vert -o %2/fullscreen_vert.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_view.frag -o %2/debug_view_frag.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/cull.comp -o %2/cull_comp.spv 
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/compact_draws.comp -o %2/compact_draws_comp.spv 

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/pp.vert -o %2/pp_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/pp_color_inv.frag -o %2/pp_color_inv_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/pp_flip.frag -o %2/pp_flip_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/pp_blur.frag -o %2/pp_blur_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/pp_fxaa.frag -o %2/pp_fxaa_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/grass.vert -o %2/grass_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/grass.frag -o %2/grass_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_pbr.frag -o %2/deferred_pbr_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_cluster.comp -o %2/deferred_cluster_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_tiled.comp -o %2/deferred_tiled_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/ssao.comp -o %2/ssao_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/ssao_blur.comp -o %2/ssao_blur_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/hiz.comp -o %2/hiz_comp.spv 
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rgen -o %2/shadow_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rmiss -o %2/shadow_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rchit -o %2/shadow_rchit.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_spheres.vert -o %2/debug_spheres_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_spheres.frag -o %2/debug_spheres_frag.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/irradiance_probe_update.rgen -o %2/irradiance_probe_update_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/irradiance_probe_update.rmiss -o %2/irradiance_probe_update_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/irradiance_probe_update.rchit -o %2/irradiance_probe_update_rchit.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/surfel_update.rgen -o %2/surfel_update_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/surfel_update.rmiss -o %2/surfel_update_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/surfel_update.rchit -o %2/surfel_update_rchit.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/ssgi.rgen -o %2/ssgi_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/ssgi.rmiss -o %2/ssgi_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/ssgi.rchit -o %2/ssgi_rchit.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/bilateral_filter.comp -o %2/bilateral_filter_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/surfel_conv.comp -o %2/surfel_conv_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/probe_conv.comp -o %2/probe_conv_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/mirror_reflections.rgen -o %2/mirror_reflections_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/mirror_reflections.rmiss -o %2/mirror_reflections_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/mirror_reflections.rchit -o %2/mirror_reflections_rchit.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/bloom_prefilter.comp -o %2/bloom_prefilter_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/bloom_downsample.comp -o %2/bloom_downsample_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/bloom_upsample.comp -o %2/bloom_upsample_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/bloom_composite.comp -o %2/bloom_composite_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/tonemap.comp -o %2/tonemap_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/light_shadow.rgen -o %2/light_shadow_rgen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/light_shadow.rmiss -o %2/light_shadow_rmiss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/light_shadow.rchit -o %2/light_shadow_rchit.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/light_shadow_sum.comp -o %2/light_shadow_sum_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/particle_update.comp -o %2/particle_update_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/tlas_update.comp -o %2/tlas_update_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/surfel_sh.comp -o %2/surfel_sh_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/luminance_histogram.comp -o %2/luminance_histogram_comp.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/luminance_average.comp -o %2/luminance_average_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/imgui_fixup.frag -o %2/imgui_fixup_frag.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/terrain.vert -o %2/terrain_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/terrain.frag -o %2/terrain_frag.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/terrain_shadow.vert -o %2/terrain_shadow_vert.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/grass_gen.comp -o %2/grass_gen_comp.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_lines.vert -o %2/debug_lines_vert.spv
%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_lines.frag -o %2/debug_lines_frag.spv

%VULKAN_SDK%/Bin/glslc.exe --target-spv=spv1.4 %1/debug_geometry.vert -o %2/debug_geometry_vert.spv