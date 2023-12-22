#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "scene_ubo.glsl"
#include "bindless.glsl"

struct hitPayload
{
  vec3 irradiance;
  vec3 hitPos;
  float shadow;
};

layout(location = 1) rayPayloadInEXT hitPayload payload;

void main()
{
  payload.shadow = 0.0;
  payload.irradiance = toLinear(ubo.skyIntensity * ubo.skyColor).rgb;
}