#include "logic/StageApplication.h"

#include "util/noiseutils.h"
#include <noise/noise.h>

int main()
{
  StageApplication app("Stage");

  if (!app.init()) {
    return -1;
  }

  app.run();
  return 0;

  /*module::Perlin perlin;
  utils::NoiseMap heightMap;

  utils::NoiseMapBuilderPlane heightMapBuilder;
  heightMapBuilder.SetSourceModule(perlin);
  heightMapBuilder.SetDestNoiseMap(heightMap);
  heightMapBuilder.SetDestSize(256, 256);
  heightMapBuilder.SetBounds(2.0, 6.0, 1.0, 5.0);
  heightMapBuilder.Build();

  utils::RendererImage renderer;
  utils::Image image;
  renderer.SetSourceNoiseMap(heightMap);
  renderer.SetDestImage(image);

  renderer.Render();

  utils::WriterBMP writer;
  writer.SetSourceImage(image);
  writer.SetDestFilename("tutorial.bmp");
  writer.WriteDestFile();*/
}