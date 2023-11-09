#include "UpdateBlasPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"

namespace render {

UpdateBlasPass::UpdateBlasPass()
  : RenderPass()
{}

UpdateBlasPass::~UpdateBlasPass()
{}

void UpdateBlasPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // This will go through all dynamic (typically animated but maybe in the future also procedurally generated)
  // meshes and update their blases

  RenderPassRegisterInfo info{};
  info._name = "BLASUpdate";

  {
    ResourceUsage usage{};
    usage._resourceName = "GigaVtxBuffer";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    usage._ownedByEngine = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  fgb.registerRenderPass(std::move(info));
  fgb.registerRenderPassExe("BLASUpdate",
    [this](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;

      auto& dynamicBlases = exeParams.rc->getDynamicBlases();
      auto& rends = exeParams.rc->getCurrentRenderables();
      if (dynamicBlases.empty() || rends.empty()) return;

      std::vector<VkBufferMemoryBarrier> memBarriers;
      std::vector<std::vector<VkAccelerationStructureBuildRangeInfoKHR>> buildRangeInfos;
      std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
      std::vector<VkAccelerationStructureGeometryKHR> geometries;

      // Pre-pass to determine sizes
      /*std::size_t numUpdates = 0;
      for (auto& dynamicPair : dynamicBlases) {
        numUpdates += dynamicPair.second.size();
      }

      memBarriers.reserve(numUpdates);
      buildRangeInfos.reserve(numUpdates);
      buildGeometryInfos.reserve(numUpdates);
      geometries.reserve(numUpdates);*/

      //std::size_t rendIdx = 0;
      for (auto& dynamicPair : dynamicBlases) {
        auto rendId = dynamicPair.first;
        internal::InternalRenderable* renderable = nullptr;

        if (!exeParams.rc->getRenderableById(rendId, &renderable)) {
          continue;
        }

        assert(renderable->_meshes.size() == dynamicPair.second.size() &&
          "Dynamic blas and stored renderable don't match number of meshes!");

        assert(renderable->_rtFirstDynamicMeshId != INVALID_ID &&
          "Dynamic blas update cannot run on a renderable without dynamic mesh id set!");

        // TODO: The vector assumes to be same length as num meshes of the renderable...
        //       Feels un-robust
        for (std::size_t i = 0; i < dynamicPair.second.size(); ++i) {
          internal::InternalMesh* mesh = nullptr;

          render::MeshId meshId = renderable->_rtFirstDynamicMeshId + (MeshId)i;

          if (!exeParams.rc->getMeshById(meshId, &mesh)) {
            printf("Could not get mesh in BLAS update!\n");
            continue;
          }

          // TODO: This code below is a copy from VulkanRenderer. Abstract somewhere.

          // Now we create a structure for the triangle and index data for this mesh
          // Retrieve gigabuffer device address

          // According to spec it is ok to get address of offset data using a simple uint64_t offset
          // Also note, the vertexoffset and indexoffset are not _bytes_ but "numbers"
          VkDeviceAddress vertexAddress = exeParams.rc->getGigaVtxBufferAddr() + mesh->_vertexOffset * sizeof(Vertex);
          VkDeviceAddress indexAddress = exeParams.rc->getGigaIdxBufferAddr() + (uint32_t)mesh->_indexOffset * sizeof(uint32_t);

          VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
          triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
          triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
          triangles.vertexData.deviceAddress = vertexAddress;
          triangles.vertexStride = sizeof(Vertex);
          triangles.indexType = VK_INDEX_TYPE_UINT32;
          triangles.indexData.deviceAddress = indexAddress;
          triangles.maxVertex = mesh->_numVertices - 1;
          triangles.transformData = { 0 }; // No transform

          // Point to the triangle data in the higher abstracted geometry structure (can contain other things than triangles)
          {
            geometries.emplace_back();
            geometries.back().sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometries.back().pNext = nullptr;
            geometries.back().geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometries.back().geometry.triangles = triangles;
            geometries.back().flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
          }

          // Range information for the build process
          {
            buildRangeInfos.emplace_back();
            buildRangeInfos.back().emplace_back();
            buildRangeInfos.back().back().firstVertex = 0;
            buildRangeInfos.back().back().primitiveCount = mesh->_numIndices / 3; // Number of triangles
            buildRangeInfos.back().back().primitiveOffset = 0;
            buildRangeInfos.back().back().transformOffset = 0;
          }

          {
            auto& blas = dynamicPair.second[i];
            buildGeometryInfos.emplace_back();
            buildGeometryInfos.back().sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildGeometryInfos.back().pNext = nullptr;
            buildGeometryInfos.back().flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
            buildGeometryInfos.back().geometryCount = 1;
            buildGeometryInfos.back().pGeometries = &geometries.back();
            buildGeometryInfos.back().ppGeometries = nullptr;
            buildGeometryInfos.back().mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            buildGeometryInfos.back().type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildGeometryInfos.back().srcAccelerationStructure = blas._as;
            buildGeometryInfos.back().dstAccelerationStructure = blas._as;
            buildGeometryInfos.back().scratchData.deviceAddress = blas._scratchAddress;
          }    

          std::vector<VkAccelerationStructureBuildRangeInfoKHR*> vptrs(buildRangeInfos.size(), nullptr);
          int c = 0;
          for (auto& vec : buildRangeInfos) {
            vptrs[c++] = vec.data();
          }

          auto* pInfos = buildGeometryInfos.data();

          vkext::vkCmdBuildAccelerationStructuresKHR(
            *exeParams.cmdBuffer,
            (uint32_t)buildGeometryInfos.size(),
            pInfos,
            vptrs.data());

          // Barrier
          {
            auto& blas = dynamicPair.second[i];
            VkBufferMemoryBarrier postBarrier{};
            postBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            postBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            postBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            postBarrier.buffer = blas._buffer._buffer;
            postBarrier.offset = 0;
            postBarrier.size = VK_WHOLE_SIZE;

            memBarriers.emplace_back(std::move(postBarrier));
          }

          buildGeometryInfos.clear();
          buildRangeInfos.clear();
        }

        //rendIdx++;
      }

      // NOTE: It would seem more optimal to do all BLAS updates down here at the end,
      //       but I couldn't see any performance gains doing this.
      //       The reason for not doing it is that the validation layers suddenly
      //       take 10ms CPU time if I do it this way.

      // convert vec of vec to ptr to ptr
      /*std::vector<VkAccelerationStructureBuildRangeInfoKHR*> vptrs(buildRangeInfos.size(), nullptr);
      int c = 0;
      for (auto& vec : buildRangeInfos) {
        vptrs[c++] = vec.data();
      }

      auto* pInfos = buildGeometryInfos.data();

      vkext::vkCmdBuildAccelerationStructuresKHR(
        *exeParams.cmdBuffer,
        (uint32_t)buildGeometryInfos.size(),
        pInfos,
        vptrs.data());*/

      vkCmdPipelineBarrier(*exeParams.cmdBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0,
        0, nullptr,
        (uint32_t)memBarriers.size(), memBarriers.data(),
        0, nullptr);

    });
}

}