#version 450
layout(location = 0) in vec2 fragTexCoords;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform constants {
uint samplerId;
} pushConstants;
layout(set = 1, binding = 0) uniform sampler2D tex0; 
layout(set = 1, binding = 1) uniform sampler2D tex1; 
layout(set = 1, binding = 2) uniform sampler2D tex2; 
layout(set = 1, binding = 3) uniform sampler2D tex3; 
layout(set = 1, binding = 4) uniform sampler2D tex4; 
layout(set = 1, binding = 5) uniform sampler2D tex5; 
layout(set = 1, binding = 6) uniform sampler2D tex6; 
layout(set = 1, binding = 7) uniform sampler2D tex7; 
void main() {
vec4 val = vec4(0.0);
if (pushConstants.samplerId == 0) {
val = texture(tex0, fragTexCoords);
}
if (pushConstants.samplerId == 1) {
val = texture(tex1, fragTexCoords);
}
if (pushConstants.samplerId == 2) {
val = texture(tex2, fragTexCoords);
}
if (pushConstants.samplerId == 3) {
val = texture(tex3, fragTexCoords);
}
if (pushConstants.samplerId == 4) {
val = texture(tex4, fragTexCoords);
}
if (pushConstants.samplerId == 5) {
val = texture(tex5, fragTexCoords);
}
if (pushConstants.samplerId == 6) {
val = texture(tex6, fragTexCoords);
}
if (pushConstants.samplerId == 7) {
val = texture(tex7, fragTexCoords);
}
outColor = vec4(val);
}
