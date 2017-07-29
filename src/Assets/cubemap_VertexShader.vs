#version 330

uniform mat4 P;
uniform mat4 V;
in vec3 position;

out vec3 UV;

void main() {
    vec4 pos = P * mat4(mat3(V)) * vec4(position, 1.0);
    gl_Position = pos.xyww;
	UV = position;
}
