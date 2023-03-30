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
  vec4 viewVector;
  float time;
} ubo;

struct GrassObj
{
  vec4 worldPos;
  vec4 data; // x and y: facing dir, z: tilt, w: bend
};

layout(std430, set = 1, binding = 0) readonly buffer GrassObjBuffer {
  GrassObj blades[];
} grassObjBuffer;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out float fragT;
layout(location = 2) out vec3 fragPosition;

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
  vec4 worldPos = grassObjBuffer.blades[gl_InstanceIndex].worldPos;
  float height = worldPos.w;
  vec2 facing2D = grassObjBuffer.blades[gl_InstanceIndex].data.xy;
  vec3 facing = vec3(facing2D.x, 0.0, facing2D.y);
  float tilt = grassObjBuffer.blades[gl_InstanceIndex].data.z;
  float bend = grassObjBuffer.blades[gl_InstanceIndex].data.w;

  float width = 0.02;

  // Do simple animation of tilt and bend
  //tilt += sin(ubo.time*3.5)*0.05;
  //bend += sin(ubo.time*1.5)*0.1;

  // Calculate the width direction in which to step vertex
  vec3 widthDir = vec3(0.0, 0.0, 0.0);
  widthDir.x = facing.z;
  widthDir.z = -facing.x;

  // Find control points of the bezier curve
  vec3 p0 = worldPos.xyz;

  // End point, or tip, is dictated by base point and the tilt
  vec3 p3 = p0 + facing * tilt + up * height;

  // p1 is straight above p0
  vec3 p1 = p0 + up * height*bend;

  vec3 bladeDir = normalize(p3 - p0);

  // The shaping control point is dictated by the bend parameter
  vec3 p2 = p0 + 0.5 * bladeDir;
  p2 = p2 + up * bend;

  // Attempt animation, find vector perpendicular to blade dir
  vec3 away = cross(-widthDir, bladeDir);
  p3 = p3 + sin(ubo.time*3.0)*0.2*away;
  p2 = p2 + sin(ubo.time*3.0 + 1.57)*0.1*away;

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
    b = b + width * (1.0 - t*t) * signFlip * widthDir;
    fragNormal = fragNormal + signFlip * widthDir * normalTiltStrength;
  }

  vec3 viewB = vec3(ubo.view * vec4(b, 1.0));

  // Tilt vertices in view space if orthogonal to us
  // TODO: Fix this.. look at all the normalize()-calls OMEGALUL
  if (gl_VertexIndex != NUM_VERT_IDX) {
    vec3 camToVtx = normalize(b - ubo.cameraPos.xyz);
    camToVtx.y = 0;
    camToVtx = normalize(camToVtx);
    vec3 viewProj = ubo.viewVector.xyz;
    viewProj.y = 0;
    viewProj = normalize(viewProj);
    float d0 = dot(camToVtx, viewProj);

    vec3 viewNormal = normalize(mat3(ubo.view) * preTiltNormal);
    vec3 viewForward = vec3(0.0, 0.0, 1.0);

    vec3 viewRight = vec3(1.0, 0.0, 0.0);
    float d1 = dot(viewNormal, viewForward);
    if (abs(d0) < 1.15 && abs(d0) > 0.85) {
      if (abs(d1) < 0.1) {
        viewB = viewB + viewRight * signFlip * .01;
      }
    }
  }

  fragNormal = normalize(fragNormal);
  fragT = t;
  fragPosition = b;

  gl_Position = ubo.proj * vec4(viewB, 1.0);
}