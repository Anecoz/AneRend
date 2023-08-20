#extension GL_GOOGLE_include_directive : enable

#include "probe_helpers.glsl"

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

  vec2 probeTexStart = vec2(
    (float(probePixelStart.x) + 0.5) / float(probeTexSize.x),
    (float(probePixelStart.y) + 0.5) / float(probeTexSize.y));

  vec2 probeTexEnd = vec2(
    (float(probePixelEnd.x) + 0.5) / float(probeTexSize.x),
    (float(probePixelEnd.y) + 0.5) / float(probeTexSize.y));

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
  vec3 camOffset = vec3(floor(ubo.cameraPos.x) - float(NUM_PROBES_PER_PLANE) / 2.0, 0.0, floor(ubo.cameraPos.z) - float(NUM_PROBES_PER_PLANE) / 2.0);

  int probeX = clamp(int(floor(worldPos.x - camOffset.x)), 0, NUM_PROBES_PER_PLANE - 1);
  int probeY = clamp(int(floor(worldPos.y / 2.0)), 0, NUM_PROBE_PLANES - 1);
  int probeZ = clamp(int(floor(worldPos.z - camOffset.z)), 0, NUM_PROBES_PER_PLANE - 1);

  vec3 baseProbePos = vec3(probeX + camOffset.x, probeY * 2.0, probeZ + camOffset.z);

  float sumWeight = 0.0;
  vec3 sumIrradiance = vec3(0.0);

  // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
  vec3 alpha = clamp((worldPos - baseProbePos) / PROBE_STEP, vec3(0.0), vec3(1.0));

  vec3 probePos = baseProbePos;

  vec4 res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY, probeZ), alpha, ivec3(0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 1.0, 1.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY + 1, probeZ + 1), alpha, ivec3(1, 1, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 1.0, 0.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY + 1, probeZ), alpha, ivec3(1, 1, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 0.0, 0.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX + 1, probeY, probeZ), alpha, ivec3(1, 0, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 1.0, 1.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY + 1, probeZ + 1), alpha, ivec3(0, 1, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 1.0, 0.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY + 1, probeZ), alpha, ivec3(0, 1, 0));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(0.0, 0.0, 1.0);
  res = weightProbe(probeTex, worldPos, probePos, normal, ivec3(probeX, probeY, probeZ + 1), alpha, ivec3(0, 0, 1));
  sumIrradiance += res.w * res.rgb;
  sumWeight += res.w;

  probePos = baseProbePos + vec3(1.0, 0.0, 1.0);
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

vec3 calcIndirectSpecularLight(
  vec3 normal,
  vec3 albedo,
  float metallic,
  float roughness,
  vec3 worldPos,
  vec2 texCoords,
  sampler2D specTex,
  sampler2D lutTex)
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
  const float MAX_REFLECTION_LOD = 3.0;

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

  //vec3 refl = textureLod(specTex, texCoords, finalLod).rgb * clamp(1.0 - roughness, 0.2, 1.0);
  vec3 refl = textureLod(specTex, texCoords, finalLod).rgb;
  //vec3 refl = textureLod(specTex, texCoords, 0).rgb;
  vec2 envBRDF = texture(lutTex, vec2(max(dot(normal, V), 0.0), 1.01 - roughness)).rg;
  vec3 specular = refl * (kS * envBRDF.x + envBRDF.y);

  return specular * 0.5;
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