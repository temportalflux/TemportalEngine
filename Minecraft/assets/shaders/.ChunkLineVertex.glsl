#version 450
#extension GL_ARB_separate_shader_objects : enable

// Camera-based unform - changes each frame based on the camera's POV and chunk data
layout(set = 0, binding = 0) uniform CameraUniform {
	mat4 view;
	mat4 proj;
	vec3 posOfCurrentChunk;
	vec3 chunkSize;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragColor;

void main()
{	
	gl_Position = camera.proj * camera.view * vec4(position, 1.0);
	fragColor = color;
}
