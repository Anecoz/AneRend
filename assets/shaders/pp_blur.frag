#version 450

layout(binding = 0) uniform sampler2D inputTex;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.411764705882353) * direction;
  vec2 off2 = vec2(3.2941176470588234) * direction;
  vec2 off3 = vec2(5.176470588235294) * direction;
  color += texture(image, uv) * 0.1964825501511404;
  color += texture(image, uv + (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv - (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv + (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv - (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv + (off3 / resolution)) * 0.010381362401148057;
  color += texture(image, uv - (off3 / resolution)) * 0.010381362401148057;
  return color;
}

vec4 blur9(vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.3846153846) * direction;
  vec2 off2 = vec2(3.2307692308) * direction;
  color += texture(inputTex, uv) * 0.2270270270;
  color += texture(inputTex, uv + (off1 / resolution)) * 0.3162162162;
  color += texture(inputTex, uv - (off1 / resolution)) * 0.3162162162;
  color += texture(inputTex, uv + (off2 / resolution)) * 0.0702702703;
  color += texture(inputTex, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}

void main() {
  outColor = blur13(inputTex, fragTexCoords, textureSize(inputTex, 0), vec2(1.0, 0.0));
  //outColor = blur9(fragTexCoords, textureSize(inputTex, 0), vec2(1.0, 0.0));
}