#version 330

uniform mat4 P;
uniform mat4 V;
in vec3 position;

void main() {
    gl_Position = P * V * vec4(position, 1.0);
}
