#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"

struct GrassObj
{
  vec4 worldPos;
  vec4 cpData0;
  vec4 cpData1;
  vec4 cpData2;
  vec4 widthDir;
};

layout(std430, set = 1, binding = 0) readonly buffer GrassObjBuffer {
  GrassObj blades[];
} grassObjBuffer;

layout(push_constant) uniform constants {
  uint shadow; // if 1 use shadow matrices, else not
} pushConstants;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out float fragT;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out flat float fragBladeHash;

vec3 cubeBezier(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t)
{
  return (1.0 - t)*(1.0 - t)*(1.0 - t)*p0 + 3.0 * (1.0 - t)*(1.0 - t)*t*p1 + 3.0*(1.0 - t)*t*t*p2 + t*t*t*p3;
}

vec3 cubeBezierDeriv(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t)
{
  return 3.0*(1.0 - t)*(1.0 - t)*(p1-p0) + 6.0*(1.0 - t)*t*(p2-p1) + 3.0*t*t*(p3-p2);
}

vec3 quadBezier(vec3 p0, vec3 p1, vec3 p2, float t)
{
  return (1.0-t)*(1.0-t) * p0 + 2.0*(1.0-t)*t*p1 + t*t*p2;
}

vec3 quadBezierDeriv(vec3 p0, vec3 p1, vec3 p2, float t)
{
  return 2.0 * (1.0 - t) * (p1 - p0) + 2.0*t*(p2 - p1);
}

const vec3 up = vec3(0.0, 1.0, 0.0);

const int NUM_VERT_IDX = 14;

void main() {
  mat4 view = pushConstants.shadow == 1 ? ubo.directionalShadowMatrixView : ubo.view;
  mat4 proj = pushConstants.shadow == 1 ? ubo.directionalShadowMatrixProj : ubo.proj;

  float width = 0.01;

  // Reconstruct control points from grass blade data
  vec3 p0 = grassObjBuffer.blades[gl_InstanceIndex].cpData0.xyz;
  vec3 p1 = vec3(grassObjBuffer.blades[gl_InstanceIndex].cpData0.w, grassObjBuffer.blades[gl_InstanceIndex].cpData1.xy);
  vec3 p2 = vec3(grassObjBuffer.blades[gl_InstanceIndex].cpData1.zw, grassObjBuffer.blades[gl_InstanceIndex].cpData2.x);
  vec3 p3 = grassObjBuffer.blades[gl_InstanceIndex].cpData2.yzw;

  vec3 widthDir = grassObjBuffer.blades[gl_InstanceIndex].widthDir.xyz;
  float bladeHash = grassObjBuffer.blades[gl_InstanceIndex].widthDir.w;

  // Use bezier function to find this vertex' point
  float t = floor(float(gl_VertexIndex) / 2.0) / (float(NUM_VERT_IDX) / 2.0);

  vec3 b = cubeBezier(p0, p1, p2, p3, t);

  // Calculate normal using the width dir and derivative of bezier curve
  vec3 deriv = cubeBezierDeriv(p0, p1, p2, p3, t);
  fragNormal = normalize(cross(widthDir, deriv));

  vec3 preTiltNormal = fragNormal;

  // Step the point in width direction, also tilt normal outwards
  float normalTiltStrength = 0.5;
  float signFlip =  mod(gl_VertexIndex, 2) == 0 ? -1.0 : 1.0;

  // Skip the last vertex...
  if (gl_VertexIndex != NUM_VERT_IDX) {
    b = b + width /** (1.0 - t*t)*/ * signFlip * widthDir;
    fragNormal = fragNormal + signFlip * widthDir * normalTiltStrength;
  }

  vec3 viewB = vec3(view * vec4(b, 1.0));

  // Tilt vertices in view space if orthogonal to us
  /*if (gl_VertexIndex != NUM_VERT_IDX) {
    vec3 camToVtx = normalize(b - ubo.cameraPos.xyz);
    float d = dot(preTiltNormal, camToVtx);

    vec3 viewNormal = normalize(mat3(ubo.view) * preTiltNormal);
    float dotSign = d < 0.0? 1.0 : -1.0;
    if (abs(d) < 0.5) {
      viewB = viewB + viewNormal * signFlip * dotSign * .008;
    }
  }*/

  fragNormal = normalize(fragNormal);
  fragT = t;
  fragBladeHash = bladeHash;
  fragPos = b;

  gl_Position = proj * vec4(viewB, 1.0);
}