#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D fontAtlasSampler;

layout(location = 0) out vec4 outColor;

const float width = 0.5;
const float edge = 0.1;

void main()
{
	vec4 pixel = texture(fontAtlasSampler, fragTexCoord);
	float distance = 1.0 - pixel.a;
	float alpha = 1 - smoothstep(width, width + edge, distance);
	outColor = fragColor * vec4(1, 1, 1, alpha);
}