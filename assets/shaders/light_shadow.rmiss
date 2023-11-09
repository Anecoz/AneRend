#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
  float dist;
  float maxDist;
};

layout(location = 0) rayPayloadInEXT hitPayload payload;

void main()
{
  payload.dist = payload.maxDist;
}