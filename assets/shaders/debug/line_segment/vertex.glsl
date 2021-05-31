#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex Attributes - related to a given line segment
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

// Uniform binding
layout(set = 0, binding = 0) uniform CameraViewProjection {
	mat4 view;
	mat4 proj;
} camera;

layout(location = 0) out vec4 frag_color;

void main()
{
	gl_Position = camera.proj * camera.view * vec4(in_position, 1.0);
	frag_color = in_color;
}
