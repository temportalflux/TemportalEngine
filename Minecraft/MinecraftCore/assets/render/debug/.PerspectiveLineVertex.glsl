#version 450
#extension GL_ARB_separate_shader_objects : enable

// Camera-based unform - changes each frame based on the camera's POV and chunk data
layout(set = 0, binding = 0) uniform CameraUniform {
	mat4 view;
	mat4 proj;
	vec3 posOfCurrentChunk;
	vec3 chunkSize;
} localCamera;

// each row in glsl is a column in the matrix
// this is a view matrix for an eye pos at <0, -10, 0> w/ no rotation and up=<0, 0, 1>
mat4 camera = {	
	{ 1, 0, 0, 0 },
	{ 0, 1,-1, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0,-10,1 }
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragColor;

void main()
{
	mat4 orientation = localCamera.view;
	orientation[3] = vec4(0, 0, 0, 1);
	
	gl_Position = localCamera.proj * camera * orientation * vec4(position, 1.0);
	fragColor = color;
}
