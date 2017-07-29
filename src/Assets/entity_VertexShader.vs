#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 lightSpaceMatrix;

in vec3 position;
in vec2 tileUV;

out vec2 UV;
out vec4 fragPosLightSpace;

void main() {
    gl_Position = P * V * vec4(position, 1.0);
	UV = tileUV;
	fragPosLightSpace = lightSpaceMatrix * vec4(position, 1.0);
}
