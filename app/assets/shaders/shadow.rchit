#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "bindless.glsl"

struct hitPayload
{
  float hitValue;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
  prd.hitValue = 0.0f;
}