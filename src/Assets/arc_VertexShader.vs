#version 330

// Model-Space coordinates
in vec2 position;

uniform mat4 M;

void main() {
	gl_Position = M * vec4( position, 0, 1 );
}
