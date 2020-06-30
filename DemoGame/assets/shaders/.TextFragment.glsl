#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D fontAtlasSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor * texture(fontAtlasSampler, fragTexCoord);
}