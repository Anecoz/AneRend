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
  vec4 data2; //x: per-blade hash
};

layout(std430, set = 1, binding = 0) readonly buffer GrassObjBuffer {
  GrassObj blades[];
} grassObjBuffer;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out float fragT;
layout(location = 2) out vec3 fragPosition;
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
  vec4 worldPos = grassObjBuffer.blades[gl_InstanceIndex].worldPos;
  float height = worldPos.w;
  vec2 facing2D = grassObjBuffer.blades[gl_InstanceIndex].data.xy;
  vec3 facing = vec3(facing2D.x, 0.0, facing2D.y);
  float tilt = grassObjBuffer.blades[gl_InstanceIndex].data.z;
  float bend = grassObjBuffer.blades[gl_InstanceIndex].data.w;
  float bladeHash = grassObjBuffer.blades[gl_InstanceIndex].data2.x;
  float windStrength = grassObjBuffer.blades[gl_InstanceIndex].data2.y;

  float width = 0.01;

  // Calculate the width direction in which to step vertex
  vec3 widthDir = vec3(0.0, 0.0, 0.0);
  widthDir.x = facing.z;
  widthDir.z = -facing.x;

  // Find control points of the bezier curve
  vec3 p0 = worldPos.xyz;

  // End point, or tip, is dictated by base point and the tilt
  vec3 p3 = p0 + facing * tilt + up * height;

  vec3 bladeDir = normalize(p3 - p0);
  vec3 away = cross(-widthDir, bladeDir);

  // p1 is straight above p0
  vec3 p1 = p0 + up * height * bend;

  // The shaping control point is dictated by the bend parameter
  vec3 p2 = p0 + 0.5 * (p3 - p0);
  p2 = p2 + away * bend;

  // Attempt animation, find vector perpendicular to blade dir
  float phaseOffset = bladeHash * 1.57;
  float speedMult = 2.0 * (bladeHash + 1.0) * (windStrength + 1.0);
  float maxAmplitude = 0.01 * (windStrength + 3.0);
  float timeMod = mod(ubo.time, 2.0*acos(-1.0)); // To stop loss of precision when ubo.time gets big
  p3 = p3 + sin(timeMod*speedMult + phaseOffset)*maxAmplitude*away;
  p2 = p2 + sin(timeMod*speedMult + 1.57 + phaseOffset)*maxAmplitude/2.0*away;

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

  vec3 viewB = vec3(ubo.view * vec4(b, 1.0));

  // Tilt vertices in view space if orthogonal to us
  if (gl_VertexIndex != NUM_VERT_IDX) {
    vec3 camToVtx = normalize(b - ubo.cameraPos.xyz);
    float d = dot(preTiltNormal, camToVtx);

    vec3 viewRight = vec3(1.0, 0.0, 0.0);
    float dotSign = d < 0.0? 1.0 : -1.0;
    if (abs(d) < 0.5) {
      viewB = viewB + viewRight * signFlip * dotSign * .005;
    }
  }

  fragNormal = normalize(fragNormal);
  fragT = t;
  fragPosition = b;
  fragBladeHash = bladeHash;

  gl_Position = ubo.proj * vec4(viewB, 1.0);
}