#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragFlags;

// BlockType-based unform - bound based on which block type is being drawn
layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	if (fragFlags.x == 0) discard;
	// Actual
	outColor = fragColor * texture(texSampler, fragTexCoord);

}
