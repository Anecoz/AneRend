#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
  vec3 irradiance;
  vec3 hitPos;
  bool shadow;
};

layout(location = 1) rayPayloadInEXT hitPayload payload;

void main()
{
  payload.shadow = false;
  payload.irradiance = vec3(0.2, 0.5, 0.7);
}