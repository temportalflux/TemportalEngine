#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec4 frag_color;

layout(set = 1, binding = 0) uniform sampler2D boid_texture;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = frag_color * texture(boid_texture, frag_tex_coord);	
}