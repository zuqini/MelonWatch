#version 330

in vec3 UV;
uniform samplerCube skybox;
uniform bool render_textures;

out vec4 fragColor;

void main() {
	fragColor = render_textures ? texture(skybox, UV) : vec4(0.8, 0.8, 0.8, 1.0);
}
