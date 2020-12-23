#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	gl_Position = vec4(inPos.xy, 0, 1.0);
	fragColor = vec4(1, 1, 1, 1.0);
	fragTexCoord = inTexCoord;
}