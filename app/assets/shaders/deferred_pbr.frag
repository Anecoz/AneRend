#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
  uint screenWidth;
  uint screenHeight;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D im0;
layout(set = 1, binding = 1) uniform sampler2D im1;
layout(set = 1, binding = 2) uniform sampler2D depth;
layout(set = 1, binding = 3) uniform sampler2D shadowMap;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outCol;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
   float a      = roughness*roughness;
   float a2     = a*a;
   float NdotH  = max(dot(N, H), 0.0);
   float NdotH2 = NdotH*NdotH;

   float num   = a2;
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

float inShadow(vec3 worldPos) {
  vec4 shadowPos = ubo.directionalShadowMatrixProj * ubo.directionalShadowMatrixView * vec4(worldPos, 1.0);
  vec3 projCoords = shadowPos.xyz / shadowPos.w;

  if (projCoords.x > 1.0 || projCoords.x < -1.0 ||
      projCoords.z > 1.0 || projCoords.z < -1.0 ||
      projCoords.y > 1.0 || projCoords.y < -1.0) {
    return 0;
  }

  vec2 shadowMapCoord = projCoords.xy * 0.5 + 0.5;

  float depth = projCoords.z;
  float sampledDepth = texture(shadowMap, shadowMapCoord.xy).r;
  if (depth > sampledDepth) {
    return 1;
  }
  return 0;
}

vec3 calcWorldPos(vec2 texCoord, float depthSamp)
{
  vec4 clipSpacePos = vec4(texCoord * 2.0 - vec2(1.0), depthSamp, 1.0);

  //vec4 position = inverse_projection_matrix * clip_space_position; // Use this for view space
  vec4 position = ubo.invViewProj * clipSpacePos; // Use this for world space

  return(position.xyz / position.w);
}


void main() {
  vec4 im0Samp = texture(im0, fragTexCoords);
  vec4 im1Samp = texture(im1, fragTexCoords);
  float depthSamp = texture(depth, fragTexCoords).r;

  if (depthSamp == 1.0f) discard;

  vec3 normal = im0Samp.rgb;
  vec3 albedo = pow(vec3(im0Samp.a, im1Samp.rg), vec3(2.2));
  float metallic = im1Samp.b;
  float roughness = im1Samp.a;
  float depth = depthSamp;
  vec3 fragWorldPos = calcWorldPos(fragTexCoords, depthSamp);

  vec3 color = vec3(0.0f);
  vec3 lightColor = vec3(1.0);

  /* Implementation from https://learnopengl.com/PBR/Theory */
  vec3 N = normal;
  vec3 V = normalize(ubo.cameraPos.xyz - fragWorldPos);
  vec3 R = reflect(V, N);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // Reflectance equation
  vec3 Lo = vec3(0.0);

  for (int i = 0; i < 5; ++i) {
    float attenuation = 0.0f;
    vec3 L = vec3(0.0);

    //if (i == 0) {
      attenuation = 1.0f; // Directional light
      L = normalize(ubo.lightDir.xyz * vec3(-1.0, 1.0, -1.0));
    //}
    /*else {
      float d = 0.1 * distance(ubo.lightPos[i-1].xyz, fragWorldPos);
      attenuation = 1.0 / (d * d);
      L = normalize(ubo.lightPos[i-1].xyz - fragWorldPos);
      lightColor = ubo.lightColor[i-1].rgb;
    }*/

    vec3 H = normalize(V + L);
    vec3 radiance = lightColor * attenuation;

    // Cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  float shadow = inShadow(fragWorldPos);

  vec3 ambient = vec3(0.03) * albedo;
  color = (ambient + Lo) * clamp(1.0 - shadow, 0.3, 1.0);

  // HDR tonemapping
  color = color / (color + vec3(1.0));
  // gamma correct
  color = pow(color, vec3(1.0/2.2)); 

  outCol = vec4(color, 1.0);
}