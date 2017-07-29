#version 330

in vec2 UV;
in vec4 fragPosLightSpace;

uniform vec4 colour;
uniform bool render_textures;
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

out vec4 fragColor;
void main() {
	vec3 color = render_textures ? texture( textureSampler, UV ).rgb : colour.rgb;
	float visibility = 1.0;
	float bias = 0.000733;
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(float x = -1.0; x <= 1.0; ++x) {
		for(float y = -1.0; y <= 1.0; ++y) {
			float pcfDepth = texture(shadowMap, fragPosLightSpace.xy + vec2(x, y) * texelSize).r;
			shadow += fragPosLightSpace.z - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;
    vec3 lighting = 0.2 * color + (1.0 - shadow) * 0.8f * color;
	fragColor = vec4(lighting, colour.a);
}
