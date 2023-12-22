#define UBO_SSAO_FLAG 1
#define UBO_FXAA_FLAG 1 << 1
#define UBO_DIRECTIONAL_SHADOWS_FLAG 1 << 2
#define UBO_RT_SHADOWS_FLAG 1 << 3
#define UBO_DDGI_FLAG 1 << 4
#define UBO_DDGI_MULTI_FLAG 1 << 5
#define UBO_SPECULAR_GI_FLAG 1 << 6
#define UBO_SS_PROBES_FLAG 1 << 7
#define UBO_BS_VIS_FLAG 1 << 8
#define UBO_HACK_FLAG 1 << 9
#define UBO_RT_ON_FLAG 1 << 10
#define UBO_BAKE_MODE_ON_FLAG 1 << 11

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invView;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  vec4 cameraPos;
  ivec4 cameraGridPos;
  vec4 lightDir;
  ivec4 bakeTileInfo; // x, y: tile idx, z: tile size
  vec4 skyColor;
  float time;
  float delta;
  uint screenWidth;
  uint screenHeight;
  float sunIntensity;
  float skyIntensity;
  float exposure;
  uint flags;
  float bloomThresh;
  float bloomKnee;
} ubo;

bool checkUboFlag(uint flag)
{
  return ((ubo.flags & flag) == flag);
}