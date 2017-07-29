#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
in vec4 colour;
in vec3 position;
in vec4 offsetSize;
out vec4 vertOutColor;

void main() {
	vec3 particlePos = offsetSize.w * position + vec3(offsetSize.x, offsetSize.y, offsetSize.z);
	gl_Position = P * V * M * vec4(particlePos, 1.0);
	vertOutColor = colour;
}
