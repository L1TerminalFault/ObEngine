#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
} pcs;

void main() {
    gl_Position = pcs.modelMatrix * vec4(inPosition, 1.0);
    
    fragColor = inColor;
}
