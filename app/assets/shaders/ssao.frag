#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
  uint screenWidth;
  uint screenHeight;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D gbuffer0Tex;
layout(set = 1, binding = 1) uniform sampler2D gbuffer2Tex;
layout(set = 1, binding = 2) uniform sampler2D noiseTex;

layout(set = 1, binding = 3) uniform UniformBufferObjectSamples {
  vec4 samples[64];
} samplesUbo;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

/*vec3 calcViewSpacePos(vec2 texCoord, float depthSamp)
{
  vec4 clipSpacePos = vec4(texCoord * 2.0 - vec2(1.0), depthSamp, 1.0);

  vec4 position = ubo.invProj * clipSpacePos; // Use this for view space
  //vec4 position = ubo.invViewProj * clipSpacePos; // Use this for world space

  return(position.xyz / position.w);
}*/

void main() {
  const vec2 noiseScale = vec2(float(ubo.screenWidth)/4.0, float(ubo.screenHeight)/4.0);

  vec4 im2Samp = texture(gbuffer2Tex, fragTexCoords);

  // From learnopengl.com
  vec3 fragPos   = (ubo.view * vec4(im2Samp.rgb, 1.0)).xyz;//calcViewSpacePos(fragTexCoords, depthSamp);
  vec3 normal    = mat3(ubo.view) * texture(gbuffer0Tex, fragTexCoords).rgb;
  vec3 randomVec = texture(noiseTex, fragTexCoords * noiseScale).xyz;

  vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
  vec3 bitangent = cross(normal, tangent);
  mat3 TBN       = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  int kernelSize = 64;
  float radius = 0.5;
  float bias = 0.025;
  for(int i = 0; i < kernelSize; ++i) {
    // get sample position
    vec3 samplePos = TBN * samplesUbo.samples[i].xyz; // from tangent to view-space
    samplePos = fragPos + samplePos * radius; 
    
    vec4 offset = vec4(samplePos, 1.0);
    offset      = ubo.proj * offset;    // from view to clip-space
    offset.xyz /= offset.w;               // perspective divide
    offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

    vec4 offsetim2Samp = texture(gbuffer2Tex, offset.xy);
    float sampleDepth = (ubo.view * vec4(offsetim2Samp.rgb, 1.0)).z;//calcViewSpacePos(offset.xy, offsetDepthSamp).z;
    
    float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
    occlusion       += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck; 
  }

  occlusion = 1.0 - (occlusion / kernelSize);
  outColor.r = occlusion;
}