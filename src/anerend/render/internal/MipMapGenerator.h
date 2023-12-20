#pragma once

namespace render {
  namespace asset {
    struct Texture;
  }

  class RenderContext;  
}

namespace render::internal {

struct MipMapGenerator
{
  static void generateMipMaps(render::asset::Texture& tex, render::RenderContext* rc);
};

}