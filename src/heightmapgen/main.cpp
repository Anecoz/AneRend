#include "../common/util/noiseutils.h"
#include <lodepng.h>

#include <cstdint>
#include <cmath>
#include <fstream>
#include <vector>

int main()
{
  constexpr unsigned size = 512;

  noise::module::Perlin myModule;
  noise::utils::NoiseMap heightMap;
  noise::utils::NoiseMapBuilderPlane heightMapBuilder;
  heightMapBuilder.SetSourceModule(myModule);
  heightMapBuilder.SetDestNoiseMap(heightMap);
  heightMapBuilder.SetDestSize(size, size);
  heightMapBuilder.SetBounds(2.0, 3.0, 1.0, 3.0);
  heightMapBuilder.Build();

  std::vector<std::uint16_t> converted;
  converted.resize(size * size);

  auto* slab = heightMap.GetSlabPtr();
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      float f = heightMap.GetValue(x, y);
      if (f < -1.0f) f = -1.0f;
      if (f > 1.0f) f = 1.0f;
      uint16_t v = 0;
      v = std::ceil(65535.0f * ((f / 2.0f) + 0.5f));

      converted[x + size * y] = v;
    }
  }
  
  auto* ptr = reinterpret_cast<std::uint8_t*>(converted.data());

  std::vector<std::uint8_t> data;
  // We have to flip MSB/LSB...
  data.resize(size * size * 2);
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      data[2 * size * y + 2 * x + 0] = ptr[2 * size * y + 2 * x + 1];
      data[2 * size * y + 2 * x + 1] = ptr[2 * size * y + 2 * x + 0];
    }
  }

  printf("Byte 0: %u\n", data[0]);
  printf("Byte 1: %u\n", data[1]);
  printf("Byte 2: %u\n", data[2]);
  printf("Byte 3: %u\n", data[3]);

  lodepng::State state;
  state.info_raw.bitdepth = 16;
  state.info_raw.colortype = LCT_GREY;
  state.info_png.color.bitdepth = 16;
  state.info_png.color.colortype = LCT_GREY;
  state.encoder.auto_convert = 0;

  std::vector<uint8_t> out;
  auto ret = lodepng::encode(out, data, size, size, state);
  printf("ret: %u\n", ret);

  std::ofstream ofs("test.png", std::ios::binary);
  ofs.write((const char*)out.data(), out.size());

  ofs.close();

#if 0
  noise::utils::RendererImage renderer;
  noise::utils::Image image;
  renderer.SetSourceNoiseMap(heightMap);
  renderer.SetDestImage(image);
  renderer.Render();

  noise::utils::WriterBMP writer;
  writer.SetSourceImage(image);
  writer.SetDestFilename("tutorial.bmp");
  writer.WriteDestFile();
#endif

  return 0;
}