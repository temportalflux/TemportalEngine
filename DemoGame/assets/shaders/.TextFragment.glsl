#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D fontAtlasSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 pixel = texture(fontAtlasSampler, fragTexCoord);
	//outColor = vec4(fragTexCoord.x * pixel.r, fragTexCoord.y * pixel.g, 0, pixel.a);
	outColor = fragColor * pixel;
}