#pragma once

#include "StagingBuffer.h"
#include "InternalTexture.h"
#include "InternalModel.h"
#include "InternalMesh.h"
#include "GigaBuffer.h"

#include "../RenderContext.h"

#include "../../util/Uuid.h"

namespace render::internal {

class UploadContext {
public:
  virtual ~UploadContext() {}

  virtual StagingBuffer& getStagingBuffer() = 0;
  virtual GigaBuffer& getVtxBuffer() = 0;
  virtual GigaBuffer& getIdxBuffer() = 0;
  virtual RenderContext* getRC() = 0;

  virtual void textureUploadedCB(InternalTexture tex) = 0;
  virtual void modelMeshUploadedCB(InternalMesh mesh, VkCommandBuffer cmdBuf) = 0;
};

}