#extension GL_GOOGLE_include_directive : enable

#include "probe_helpers.glsl"
#include "surfel_helpers.glsl"
#include "helpers.glsl"
#include "scene_ubo.glsl"

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
   float a      = roughness*roughness;
   float a2     = a*a;
   float NdotH  = max(dot(N, H), 0.0);
   float NdotH2 = NdotH*NdotH;

   float num   = a2 + 0.001;
   float denom = (NdotH2 * (a2 - 1.0) + 1.0);
   denom = PI * denom * denom;

   return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
   float r = (roughness + 1.0);
   float k = (r*r) / 8.0;

   float num   = NdotV;
   float denom = NdotV * (1.0 - k) + k;

   return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
   float NdotV = max(dot(N, V), 0.0);
   float NdotL = max(dot(N, L), 0.0);
   float ggx2  = GeometrySchlickGGX(NdotV, roughness);
   float ggx1  = GeometrySchlickGGX(NdotL, roughness);

   return ggx1 * ggx2;
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
   float a = 12.9898;
   float b = 78.233;
   float c = 43758.5453;
   float dt= dot(co.xy ,vec2(a,b));
   float sn= mod(dt,3.14);
   return fract(sin(sn) * c);
}

// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 hammersley2d(uint i, uint N)
{
   uint bits = (i << 16u) | (i >> 16u);
   bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
   bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
   bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
   bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
   float rdi = float(bits) * 2.3283064365386963e-10;
   return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/genbrdflut.frag
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal) 
{
   // Maps a 2D point to a hemisphere with spread based on roughness
   float alpha = roughness * roughness;
   float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
   float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
   float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
   vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

   // Tangent space
   vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
   vec3 tangentX = normalize(cross(up, normal));
   vec3 tangentY = normalize(cross(normal, tangentX));

   // Convert to world Space
   return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
   return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
   return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calcLight(
  vec3 normal,
  vec3 albedo,
  float metallic,
  float roughness,
  vec3 worldPos,
  vec3 viewPos,
  Light light,
  int directional,
  bool useSpecular)
{
  vec3 color = vec3(0.0);
  vec3 lightColor = ubo.sunIntensity * vec3(1.0);

  /* Implementation from https://learnopengl.com/PBR/Theory */
  vec3 N = normal;
  vec3 V = normalize(viewPos - worldPos);
  vec3 R = reflect(V, N);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // Reflectance equation
  vec3 Lo = vec3(0.0);

  const float brightness = 5.0;

  float attenuation = 0.0;
  vec3 L = vec3(0.0);

  if (directional == 1) {
    attenuation = 1.0; // Directional light
    L = normalize(-ubo.lightDir.xyz);// * vec3(-1.0, 1.0, -1.0));
  }
  else {
    vec3 lightWorldPos = light.worldPos.xyz;
    float d = min(distance(lightWorldPos, worldPos), light.worldPos.w);
    attenuation = (1.0 - d / light.worldPos.w)*(1.0 - d/light.worldPos.w) * brightness;
    //float d = distance(lightWorldPos, worldPos);
    //attenuation = clamp(brightness - d/(1.0/brightness * light.worldPos.w), 0.0, brightness);//1.0 / (d * d);
    L = normalize(lightWorldPos - worldPos);
    lightColor = light.color.rgb;
  }

  vec3 H = normalize(V + L);
  vec3 radiance = lightColor * attenuation;

  // Cook-torrance brdf
  float NDF = DistributionGGX(N, H, roughness);
  float G   = GeometrySmith(N, V, L, roughness);
  vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);

  vec3 numerator    = NDF * G * F;
  float denominator = 4.0 * NdotV * NdotL + 0.0001;
  vec3 specular     = clamp(numerator / denominator, 0.0, 1.0);

  // Add to outgoing radiance Lo
  if (!useSpecular) {
    specular = vec3(0.0);
  }
  Lo += (kD * albedo / PI + specular) * radiance * NdotL;

  return Lo;
}

vec3 sampleSingleProbe(sampler2D probeTex, ivec3 probeIndex, vec3 normal)
{
  ivec2 probeTexSize = textureSize(probeTex, 0);

  ivec2 probePixelStart = ivec2(
    (PROBE_CONV_PIX_SIZE + 2) * probeIndex.x + 1,
    (PROBE_CONV_PIX_SIZE + 2) * NUM_PROBES_PER_PLANE * probeIndex.y + (PROBE_CONV_PIX_SIZE + 2) * probeIndex.z + 1);

  ivec2 probePixelEnd = probePixelStart + PROBE_CONV_PIX_SIZE - 1;

  /*vec2 probeTexStart = vec2(
    (float(probePixelStart.x) + 0.5) / float(probeTexSize.x),
    (float(probePixelStart.y) + 0.5) / float(probeTexSize.y));*/

  vec2 probeTexStart = pixelToUv(probePixelStart, probeTexSize);

  /*vec2 probeTexEnd = vec2(
    (float(probePixelEnd.x) + 0.5) / float(probeTexSize.x),
    (float(probePixelEnd.y) + 0.5) / float(probeTexSize.y));*/

  vec2 probeTexEnd = pixelToUv(probePixelEnd, probeTexSize);

  // This returns on -1 to 1, so change to 0 to 1
  vec2 oct = octEncode(normalize(normal));
  oct = (oct + vec2(1.0)) * 0.5;
  //oct = clamp(oct, vec2(0.05), vec2(0.95));

  vec2 octTexCoord = probeTexStart + (probeTexEnd - probeTexStart) * oct;

  //vec4 irr = texture(probeTex, octTexCoord);
  //return irr.rgb / irr.w;
  return texture(probeTex, octTexCoord).rgb;
}

vec4 weightProbe(sampler2D probeTex, vec3 worldPos, vec3 probePos, vec3 normal, ivec3 probeIndex, vec3 alpha, ivec3 offset)
{
  vec3 trilinear = mix(1.0 - alpha, alpha, offset);
  float weight = 1.0;
  vec3 probeIrradiance = vec3(0.0);

  // Smooth backface test
  {
    // Computed without the biasing applied to the "dir" variable. 
    // This test can cause reflection-map looking errors in the image
    // (stuff looks shiny) if the transition is poor.
    vec3 trueDirectionToProbe = normalize(probePos - worldPos);

    // The naive soft backface weight would ignore a probe when
    // it is behind the surface. That's good for walls. But for small details inside of a
    // room, the normals on the details might rule out all of the probes that have mutual
    // visibility to the point. So, we instead use a "wrap shading" test below inspired by
    // NPR work.
    // weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

    // The small offset at the end reduces the "going to zero" impact
    // where this is really close to exactly opposite
    weight *= pow(max(0.0001, (dot(trueDirectionToProbe, normal) + 1.0) * 0.5), 2) + 0.2;
  }

  // Avoid zero weight
  weight = max(0.000001, weight);

  // A tiny bit of light is really visible due to log perception, so
  // crush tiny weights but keep the curve continuous. This must be done
  // before the trilinear weights, because those should be preserved.
  const float crushThreshold = 0.2;
  if (weight < crushThreshold) {
    weight *= weight * weight * (1.0 / pow(crushThreshold, 2));
  }

  // Trilinear weights
  weight *= trilinear.x * trilinear.y * trilinear.z;

  probeIrradiance += sampleSingleProbe(probeTex, probeIndex, normal);

  // Weight in a more-perceptual brightness space instead of radiance space.
  // This softens the transitions between probes with respect to translation.
  // It makes little difference most of the time, but when there are radical transitions
  // between probes this helps soften the ramp.
  probeIrradiance = sqrt(probeIrradiance);

  return vec4(probeIrradiance, weight);
}

vec3 sampleProbe(sampler2D probeTex, vec3 worldPos, vec3 normal)
{
  // Find closest probe to worldPos
  ivec2 camProbeIdx =
    ivec2(int(ubo.cameraPos.x / PROBE_STEP.x), int(ubo.cameraPos.z / PROBE_STEP.z));

  vec3 camOffset = vec3(
    float(camProbeIdx.x) * PROBE_STEP.x - PROBE_STEP.x * float(NUM_PROBES_PER_PLANE) / 2.0,
    0.0,
    float(camProbeIdx.y) * PROBE_STEP.z - PROBE_STEP.z * float(NUM_PROBES_PER_PLANE) / 2.0);
  //vec3 camOffset = vec3(ubo.cameraGridPos.x - PROBE_STEP.x * float(NUM_PROBES_PER_PLANE) / 2.0, 0.0, ubo.cameraGridPos.z - PROBE_STEP.z * float(NUM_PROBES_PER_PLANE) / 2.0);
  //vec3 camOffset = vec3(0.0);
  //vec3 camOffset = vec3(floor(ubo.cameraPos.x) - float(NUM_PROBES_PER_PLANE) / 2.0, 0.0, floor(ubo.cameraPos.z) - float(NUM_PROBES_PER_PLANE) / 2.0);

  int probeX = clamp(int(floor((worldPos.x - camOffset.x) / PROBE_STEP.x)), 0, NUM_PROBES_PER_PLANE - 1);
  int probeY = clamp(int(floor(worldPos.y / PROBE_STEP.y)), 0, NUM_PROBE_PLANES - 1);
  int probeZ = clamp(int(floor((worldPos.z - camOffset.z) / PROBE_STEP.z)), 0, NUM_PROBES_PER_PLANE - 1);

  //vec3 baseProbePos = vec3(probeX + camOffset.x, probeY * 2.0, probeZ + camOffset.z);
  vec3 baseProbePos = vec3(probeX, probeY, probeZ) * PROBE_STEP + camOffset;

  float sumWeight = 0.0;
  vec3 sumIrradiance = vec3(0.0);

  // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
  vec3 alpha = clamp((worldPos - baseProbePos) / PROBE_STEP, vec3(0.0), vec3(1.0));

  vec3 probePos = baseProbePos;

  vec4 res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY, probeZ), alpha, ivec3(0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 1.0, 1.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY + 1, probeZ + 1), alpha, ivec3(1, 1, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 1.0, 0.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY + 1, probeZ), alpha, ivec3(1, 1, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 0.0, 0.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY, probeZ), alpha, ivec3(1, 0, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 1.0, 1.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY + 1, probeZ + 1), alpha, ivec3(0, 1, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 1.0, 0.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY + 1, probeZ), alpha, ivec3(0, 1, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 0.0, 1.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY, probeZ + 1), alpha, ivec3(0, 0, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 0.0, 1.0) * PROBE_STEP;
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY, probeZ + 1), alpha, ivec3(1, 0, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  vec3 netIrradiance = sumIrradiance / sumWeight;

  // Go back to linear irradiance
  netIrradiance = pow(netIrradiance, ivec3(2));

  return netIrradiance * .5 * 3.1415926;
}

vec3 calcIndirectDiffuseLight(
  vec3 normal,
  vec3 albedo,
  float metallic,
  float roughness,
  vec3 worldPos,
  vec3 viewPos,
  sampler2D probeTex)
{
  vec3 ambient = vec3(0.0);

  vec3 V = normalize(viewPos - worldPos);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  vec3 kS = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;
  vec3 irradiance = sampleProbe(probeTex, worldPos, normal);
  vec3 diffuse = irradiance * albedo;
  ambient = kD * diffuse;

  return ambient;
}

vec3 irradianceSingleSurfelProbe(ivec2 surfelIdx, ivec2 cascShTexSize, sampler2D tex, vec3 normal)
{
  ivec2 shStartPixel = surfelIdx * 3;
  vec2 shStartUv = pixelToUv(shStartPixel, cascShTexSize);
  vec2 texelSize = 1.0 / vec2(cascShTexSize.x, cascShTexSize.y);
  vec2 stepX = vec2(texelSize.x, 0.0);
  vec2 stepY = vec2(0.0, texelSize.y);

  // Get the L_lm values from the tex, and construct the matrix M (one for each RGB-channel)
  vec3 l00 = texture(tex, shStartUv).rgb;
  vec3 l11 = texture(tex, shStartUv + stepX).rgb;
  vec3 l10 = texture(tex, shStartUv + stepX * 2.0).rgb;
  vec3 l1m1 = texture(tex, shStartUv + stepY).rgb;
  vec3 l21 = texture(tex, shStartUv + stepY + stepX).rgb;
  vec3 l2m1 = texture(tex, shStartUv + stepY + stepX * 2.0).rgb;
  vec3 l2m2 = texture(tex, shStartUv + stepY * 2.0).rgb;
  vec3 l20 = texture(tex, shStartUv + stepY * 2.0 + stepX).rgb;
  vec3 l22 = texture(tex, shStartUv + stepY * 2.0 + stepX * 2.0).rgb;

  const float c1 = 0.429043;
  const float c2 = 0.511664;
  const float c3 = 0.743125;
  const float c4 = 0.886227;
  const float c5 = 0.247708;

  vec3 n = normal;
  vec3 irradiance =
    c1 * l22 * (n.x * n.x - n.y * n.y) +
    c3 * l20 * n.z * n.z +
    c4 * l00 - c5 * l20 +
    2.0 * c1 * (l2m2 * n.x * n.y + l21 * n.x * n.z + l2m1 * n.y * n.z) +
    2.0 * c2 * (l11 * n.x + l1m1 * n.y + l10 * n.z);

  return max(irradiance, vec3(0.0));
}

vec3 irradianceFromCascade(int cascade, sampler2D tex, vec2 texCoord, vec3 normal)
{
  int pixSize = int(SURFEL_DIR_IRR_PIX_SIZE[cascade]);
  ivec2 cascTexSize = ivec2(ubo.screenWidth / SURFEL_PIXEL_SIZE[cascade] * pixSize, ubo.screenHeight / SURFEL_PIXEL_SIZE[cascade] * pixSize);
  ivec2 cascShTexSize = ivec2(ubo.screenWidth / SURFEL_PIXEL_SIZE[cascade] * 3, ubo.screenHeight / SURFEL_PIXEL_SIZE[cascade] * 3);

  ivec2 pixel = uvToPixel(texCoord, cascTexSize);
  ivec2 surfelIdx = pixel / pixSize;
  ivec2 surfelCenterPixel = surfelIdx * int(SURFEL_PIXEL_SIZE[cascade]) + int(SURFEL_PIXEL_SIZE[cascade]) / 2;

  vec3 irradiance = vec3(0.0);

  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(1, 1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(-1, -1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(-1, 1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(1, -1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(1, 0), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(0, 1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(0, -1), cascShTexSize, tex, normal);
  irradiance += irradianceSingleSurfelProbe(surfelIdx + ivec2(-1, 0), cascShTexSize, tex, normal);

  return irradiance / 8.0;
}

vec3 calcIndirectDiffuseLightSurfel(
  vec3 normal,
  vec3 albedo,
  float metallic,
  float roughness,
  vec3 worldPos,
  vec3 viewPos,
  vec2 texCoord,
  sampler2D cascade0Tex0,
  sampler2D cascade0Tex1, 
  sampler2D cascade0Tex2, 
  sampler2D cascade0Tex3, 
  sampler2D cascade0Tex4, 
  sampler2D cascade0Tex5, 
  sampler2D cascade0Tex6, 
  sampler2D cascade0Tex7, 
  sampler2D cascade0Tex8,
  sampler2D cascade1Tex0,
  sampler2D cascade1Tex1,
  sampler2D cascade1Tex2,
  sampler2D cascade1Tex3,
  sampler2D cascade1Tex4,
  sampler2D cascade1Tex5,
  sampler2D cascade1Tex6,
  sampler2D cascade1Tex7,
  sampler2D cascade1Tex8,
  sampler2D cascade2Tex0,
  sampler2D cascade2Tex1,
  sampler2D cascade2Tex2,
  sampler2D cascade2Tex3,
  sampler2D cascade2Tex4,
  sampler2D cascade2Tex5,
  sampler2D cascade2Tex6,
  sampler2D cascade2Tex7,
  sampler2D cascade2Tex8,
  sampler2D cascade3Tex0,
  sampler2D cascade3Tex1,
  sampler2D cascade3Tex2,
  sampler2D cascade3Tex3,
  sampler2D cascade3Tex4,
  sampler2D cascade3Tex5,
  sampler2D cascade3Tex6,
  sampler2D cascade3Tex7,
  sampler2D cascade3Tex8)
{
  vec3 ambient = vec3(0.0);

  vec3 V = normalize(viewPos - worldPos);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  vec3 kS = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  /*vec3 irradiance = irradianceFromCascade(0, cascade0Tex, texCoord, normal);
  irradiance += irradianceFromCascade(1, cascade1Tex, texCoord, normal);
  irradiance += irradianceFromCascade(2, cascade2Tex, texCoord, normal);
  irradiance += irradianceFromCascade(3, cascade3Tex, texCoord, normal);

  irradiance /= 4.0;*/

  vec3 l00 = texture(cascade0Tex0, texCoord).rgb;
  vec3 l11 = texture(cascade0Tex1, texCoord).rgb;
  vec3 l10 = texture(cascade0Tex2, texCoord).rgb;
  vec3 l1m1 = texture(cascade0Tex3, texCoord).rgb;
  vec3 l21 = texture(cascade0Tex4, texCoord).rgb;
  vec3 l2m1 = texture(cascade0Tex5, texCoord).rgb;
  vec3 l2m2 = texture(cascade0Tex6, texCoord).rgb;
  vec3 l20 = texture(cascade0Tex7, texCoord).rgb;
  vec3 l22 = texture(cascade0Tex8, texCoord).rgb;

  /*l00 += texture(cascade1Tex0, texCoord).rgb;
  l11   += texture(cascade1Tex1, texCoord).rgb;
  l10   += texture(cascade1Tex2, texCoord).rgb;
  l1m1  += texture(cascade1Tex3, texCoord).rgb;
  l21   += texture(cascade1Tex4, texCoord).rgb;
  l2m1  += texture(cascade1Tex5, texCoord).rgb;
  l2m2  += texture(cascade1Tex6, texCoord).rgb;
  l20   += texture(cascade1Tex7, texCoord).rgb;
  l22   += texture(cascade1Tex8, texCoord).rgb;

  l00   += texture(cascade2Tex0, texCoord).rgb;
  l11   += texture(cascade2Tex1, texCoord).rgb;
  l10   += texture(cascade2Tex2, texCoord).rgb;
  l1m1  += texture(cascade2Tex3, texCoord).rgb;
  l21   += texture(cascade2Tex4, texCoord).rgb;
  l2m1  += texture(cascade2Tex5, texCoord).rgb;
  l2m2  += texture(cascade2Tex6, texCoord).rgb;
  l20   += texture(cascade2Tex7, texCoord).rgb;
  l22   += texture(cascade2Tex8, texCoord).rgb;

  l00   += texture(cascade3Tex0, texCoord).rgb;
  l11   += texture(cascade3Tex1, texCoord).rgb;
  l10   += texture(cascade3Tex2, texCoord).rgb;
  l1m1  += texture(cascade3Tex3, texCoord).rgb;
  l21   += texture(cascade3Tex4, texCoord).rgb;
  l2m1  += texture(cascade3Tex5, texCoord).rgb;
  l2m2  += texture(cascade3Tex6, texCoord).rgb;
  l20   += texture(cascade3Tex7, texCoord).rgb;
  l22   += texture(cascade3Tex8, texCoord).rgb;

  l00   /= 4.0;
  l11   /= 4.0;
  l10   /= 4.0;
  l1m1  /= 4.0;
  l21   /= 4.0;
  l2m1  /= 4.0;
  l2m2  /= 4.0;
  l20   /= 4.0;
  l22   /= 4.0;*/

  const float c1 = 0.429043;
  const float c2 = 0.511664;
  const float c3 = 0.743125;
  const float c4 = 0.886227;
  const float c5 = 0.247708;

  vec3 n = normal;
  vec3 irradiance =
    c1 * l22 * (n.x * n.x - n.y * n.y) +
    c3 * l20 * n.z * n.z +
    c4 * l00 - c5 * l20 +
    2.0 * c1 * (l2m2 * n.x * n.y + l21 * n.x * n.z + l2m1 * n.y * n.z) +
    2.0 * c2 * (l11 * n.x + l1m1 * n.y + l10 * n.z);

  irradiance = max(irradiance, vec3(0.0));

  vec3 diffuse = irradiance * albedo;
  ambient = kD * diffuse;

  return ambient;
}

vec3 calcIndirectSpecularLight(
  vec3 normal,
  vec3 albedo,
  float metallic,
  float roughness,
  vec3 worldPos,
  vec2 texCoords,
  sampler2D specTex,
  sampler2D lutTex,
  sampler2D probeTex)
{
  // Should there be specular light here? (Attempt to minimize bleed from smaller mips)
  /*vec3 lod0Refl = textureLod(specTex, texCoords, 0).rgb;
  if (length(lod0Refl) < .1) {
    return vec3(0.0);
  }*/
  /*if (roughness > 0.5) {
    return vec3(0.0);
  }*/

  vec3 V = normalize(ubo.cameraPos.xyz - worldPos);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  vec3 kS = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
  const float MAX_REFLECTION_LOD = 4.0;

  // Mip bias for camera
  float d = distance(ubo.cameraPos.xyz, worldPos);
  float mipBias = 0.0;

  /*if (d > 15.0) {
    mipBias = 3.0;
  }
  else if (d > 10.0) {
    mipBias = 2.0;
  }
  else if (d > 5.0) {
    mipBias = 1.0;
  }*/

  float finalLod = clamp(roughness * MAX_REFLECTION_LOD + mipBias, 0.0, MAX_REFLECTION_LOD);

  vec3 refl = vec3(0.0);

  if (roughness > 0.8 && ubo.ddgiEnabled == 1) {
    vec3 dir = reflect(-V, normal);
    refl = sampleProbe(probeTex, worldPos, dir);
  }
  else {
    refl = textureLod(specTex, texCoords, finalLod).rgb;
  }
  
  vec2 envBRDF = texture(lutTex, vec2(max(dot(normal, V), 0.0), 1.01 - roughness)).rg;
  vec3 specular = refl * (kS *envBRDF.x + envBRDF.y);

  return specular;
}

// https://www.shadertoy.com/view/XsGfWV
vec3 acesTonemap(vec3 color)
{
  const mat3 m1 = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
  );
  const mat3 m2 = mat3(
    1.60475, -0.10208, -0.00327,
    -0.53108, 1.10813, -0.07276,
    -0.07367, -0.00605, 1.07602
  );
  vec3 v = m1 * color;
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 fastAcesTonemap(vec3 x)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float fastAcesTonemapLum(float x)
{
  // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return (x * (a * x + b)) / (x * (c * x + d) + e);
}

float binAvgLum(float avgLum)
{
  if (avgLum < 0.01) {
    return 0.005;
  }
  else if (avgLum < 0.05) {
    return 0.01;
  }
  else if (avgLum < .5) {
    return 0.5;
  }
  else if (avgLum < 0.7) {
    return 0.65;
  }

  return 0.8;
}