#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 directionalShadowMatrix;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
  float time;
} ubo;

struct GrassObj
{
  vec4 worldPos;
  float zOffset;
};

layout(std430, set = 1, binding = 0) readonly buffer GrassObjBuffer {
  GrassObj blades[];
} grassObjBuffer;

const float VERT_OFFSET_X = 0.02;
const float VERT_OFFSET_y = 0.15;

vec2 offsets[7] = vec2[](
  vec2(0.0, 0.0),
  vec2(VERT_OFFSET_X, 0.0),
  vec2(0.2*VERT_OFFSET_X, VERT_OFFSET_y),
  vec2(VERT_OFFSET_X*1.1, VERT_OFFSET_y),
  vec2(VERT_OFFSET_X, 2.0*VERT_OFFSET_y),
  vec2(VERT_OFFSET_X*1.5, 2.0*VERT_OFFSET_y),
  vec2(VERT_OFFSET_X*3.5, 3.0*VERT_OFFSET_y)
);

float swayStrength[7] = float[](
  0.0,
  0.0,
  0.001,
  0.001,
  0.0015,
  0.0015,
  0.01
);

layout(location = 0) out vec3 fragNormal;

mat4 rotationMatrix(vec3 axis, float angle)
{
  axis = normalize(axis);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1.0 - c;
    
  return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
              oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
              oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
              0.0,                                0.0,                                0.0,                                1.0);
}

void main() {
  vec4 worldPos = grassObjBuffer.blades[gl_InstanceIndex].worldPos;
  vec4 origWorldPos = worldPos;

  // Naive building of grass from the 7 input vertices
  vec2 offset = offsets[gl_VertexIndex];

  // Sway
  offset.y += mix(0.02, 0.08, swayStrength[gl_VertexIndex]*sin(ubo.time * 10.0));
  offset.y += swayStrength[gl_VertexIndex]*sin(ubo.time * 6.0);

  worldPos.x += offset.x;
  worldPos.y += offset.y;

  // Rotate
  float off = grassObjBuffer.blades[gl_InstanceIndex].zOffset;
  mat4 rotMat = rotationMatrix(vec3(0.0, 1.0, 0.0), off);

  // Remove translation
  worldPos = rotMat * (worldPos - origWorldPos);
  // Add translation again
  worldPos += origWorldPos;

  fragNormal = mat3(rotMat) * vec3(0.0, 0.0, 1.0);

  gl_Position = ubo.proj * ubo.view * worldPos;
}