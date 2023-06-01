C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard.vert -o %2/standard_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard_shadow.vert -o %2/standard_shadow_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard_shadow.frag -o %2/standard_shadow_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard_instanced.vert -o %2/standard_instanced_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard_instanced_shadow.vert -o %2/standard_instanced_shadow_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/standard.frag -o %2/standard_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/fullscreen.vert -o %2/fullscreen_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/debug_view.frag -o %2/debug_view_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslangValidator.exe --target-env spirv1.4 -V -o %2/cull_comp.spv %1/cull.comp
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/pp.vert -o %2/pp_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/pp_color_inv.frag -o %2/pp_color_inv_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/pp_flip.frag -o %2/pp_flip_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/pp_blur.frag -o %2/pp_blur_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/pp_fxaa.frag -o %2/pp_fxaa_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/grass.vert -o %2/grass_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/grass.frag -o %2/grass_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_pbr.frag -o %2/deferred_pbr_frag.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_cluster.comp -o %2/deferred_cluster_comp.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/deferred_tiled.comp -o %2/deferred_tiled_comp.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/ssao.comp -o %2/ssao_comp.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/ssao_blur.comp -o %2/ssao_blur_comp.spv
C:/VulkanSDK/1.3.239.0/Bin/glslangValidator.exe --target-env spirv1.4 -V -o %2/hiz_comp.spv %1/hiz.comp
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rgen -o %2/shadow_rgen.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rmiss -o %2/shadow_rmiss.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/shadow.rchit -o %2/shadow_rchit.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/debug_spheres.vert -o %2/debug_spheres_vert.spv
C:/VulkanSDK/1.3.239.0/Bin/glslc.exe --target-spv=spv1.4 %1/debug_spheres.frag -o %2/debug_spheres_frag.spv