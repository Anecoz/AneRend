#version 450

layout(set = 1, binding = 0) uniform sampler2D geomDepth;
layout(set = 1, binding = 1) uniform sampler2D shadowMap;
layout(set = 1, binding = 2) uniform sampler2D geom0;
layout(set = 1, binding = 3) uniform sampler2D geom1;
layout(set = 1, binding = 4) uniform sampler2D finalImage;
layout(set = 1, binding = 5) uniform sampler2D ssaoImage;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
  uint samplerId;
} pushConstants;

void main() {
  vec4 val = vec4(0.0);

  if (pushConstants.samplerId == 0) {
    float col = texture(geomDepth, fragTexCoords).r;
    val = vec4(col);
  }
  else if (pushConstants.samplerId == 1) {
    float col = texture(shadowMap, fragTexCoords).r;
    val = vec4(col);
  }
  else if (pushConstants.samplerId == 2) {
    val = texture(geom0, fragTexCoords);
  }
  else if (pushConstants.samplerId == 3) {
    val = texture(geom1, fragTexCoords);
  }
  else if (pushConstants.samplerId == 4) {
    val = texture(finalImage, fragTexCoords);
  }
  else if (pushConstants.samplerId == 5) {
    float col = texture(ssaoImage, fragTexCoords).r;
    val = vec4(col);
  }

  outColor = vec4(val);
}