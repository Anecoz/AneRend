vec2 pixelToUv(ivec2 pixel, ivec2 dimensions)
{
  return (2.0 * vec2(pixel) + 1.0) / (2.0 * vec2(dimensions));
}

vec2 subPixelToUv(vec2 pixel, ivec2 dimensions)
{
  return (2.0 * pixel + 1.0) / (2.0 * vec2(dimensions));
}