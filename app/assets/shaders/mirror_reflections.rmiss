#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
  vec3 irradiance;
  float dist;
  float shadow;
};

layout(location = 1) rayPayloadInEXT hitPayload payload;

void main()
{
  payload.shadow = 0.0;
  payload.dist = 1e6;
  payload.irradiance = 2.0 * vec3(0.2, 0.5, 0.7);
}