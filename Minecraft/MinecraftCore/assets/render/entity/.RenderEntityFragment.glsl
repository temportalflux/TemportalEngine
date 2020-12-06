#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

// BlockType-based unform - bound based on which block type is being drawn
layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	// Actual
	outColor = fragColor * texture(texSampler, fragTexCoord);
}
