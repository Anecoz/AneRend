#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
  float hitValue;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
  prd.hitValue = 1.0f;
}