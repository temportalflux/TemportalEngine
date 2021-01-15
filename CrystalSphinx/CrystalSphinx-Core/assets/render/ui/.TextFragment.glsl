#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 fragWidthEdge;
layout(location = 2) in vec4 fragColor;

layout(binding = 0) uniform sampler2D fontAtlasSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 pixel = texture(fontAtlasSampler, fragTexCoord);
	float distance = 1.0 - pixel.a;
	float alpha = 1 - smoothstep(fragWidthEdge.x, fragWidthEdge.x + fragWidthEdge.y, distance);
	outColor = fragColor * vec4(1, 1, 1, alpha);
}