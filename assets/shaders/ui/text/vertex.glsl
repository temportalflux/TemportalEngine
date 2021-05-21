#version 450

layout(location = 0) in vec4 inPosAndWidthEdge;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec2 fragWidthEdge;
layout(location = 2) out vec4 fragColor;

void main()
{
	gl_Position = vec4(inPosAndWidthEdge.xy, 0, 1.0);
	fragWidthEdge = inPosAndWidthEdge.zw;
	fragTexCoord = inTexCoord;
	fragColor = inColor;
}