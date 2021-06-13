#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex Attributes - related to the boid "model"
layout(location = 0) in vec2 in_vertex_pos;
layout(location = 1) in vec2 in_tex_coord;

// Instance Attributes - related to rendering a boid "model" somewhere
layout(location = 2) in mat4 in_model;
layout(location = 6) in vec4 in_color;

// Uniform binding
layout(set = 0, binding = 0) uniform CameraViewProjection {
	mat4 view;
	mat4 proj;
} camera;

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec4 frag_color;

void main()
{
	gl_Position = camera.proj * camera.view * in_model * vec4(in_vertex_pos, 0, 1.0);
	frag_tex_coord = in_tex_coord;
	frag_color = in_color;
}
