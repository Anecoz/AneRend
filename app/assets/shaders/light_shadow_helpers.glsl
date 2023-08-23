const int NUM_RAYS_PER_LIGHT = 1024;
const int LIGHT_DIR_DIST_PIX_SIZE = 32; // sqrt(NUM_RAYS_PER_LIGHT)
const int LIGHT_SHADOW_OCT_SIZE = 8;
const int MAX_NUM_LIGHTS = 1024;

float calcShadowWeight(int lightIndex, float distToLight, vec3 dirToLight, sampler2D lightMeanTex)
{
  ivec2 lightTexSize = textureSize(lightMeanTex, 0);

  ivec2 lightPixelStart = ivec2(
    (LIGHT_SHADOW_OCT_SIZE + 2) * lightIndex + 1,
    0);

  ivec2 lightPixelEnd = lightPixelStart + LIGHT_SHADOW_OCT_SIZE - 1;

  vec2 lightTexStart = pixelToUv(lightPixelStart, lightTexSize);
  vec2 lightTexEnd = pixelToUv(lightPixelEnd, lightTexSize);

  // This returns on -1 to 1, so change to 0 to 1
  vec2 oct = octEncode(normalize(-dirToLight));
  oct = (oct + vec2(1.0)) * 0.5;

  vec2 lightTexCoord = lightTexStart + (lightTexEnd - lightTexStart) * oct;

  vec2 temp = texture(lightMeanTex, lightTexCoord).rg;

  float mean = temp.x;
  float variance = abs(mean * mean - temp.y);

  // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
  // Need the max in the denominator because biasing can cause a negative displacement
  float chebyshevWeight = variance / (variance + pow(max(distToLight - mean, 0.0), 2.0) + 0.001);

  return chebyshevWeight;
}