#version 330

in vec4 vertOutColor;

out vec4 fragColor;

void main() {
	fragColor = vec4( vertOutColor );
}
