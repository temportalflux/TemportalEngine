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
layout(location = 2) in vec3 chunkSpaceMask;

layout(location = 0) out vec4 fragColor;

float lerp(float a, float b, float delta) { return (a * (1 - delta)) + (b * delta); }
vec3 lerp(vec3 a, vec3 b, vec3 delta) { return vec3(lerp(a.x, b.x, delta.x), lerp(a.y, b.y, delta.y), lerp(a.z, b.z, delta.z)); }
vec4 lerp(vec4 a, vec4 b, float delta) { return (a * (1 - delta)) + (b * delta); }

void main()
{
	// This is the vertex position shifted out of the camera's chunk and into world space
	vec3 pos_wrt_rootChunk = position + ((vec3(0, 0, 0) - camera.posOfCurrentChunk) * camera.chunkSize);
	vec3 vertPos = lerp(pos_wrt_rootChunk, position, chunkSpaceMask);
	gl_Position = camera.proj * camera.view * vec4(vertPos, 1.0);
	fragColor = color;
}
