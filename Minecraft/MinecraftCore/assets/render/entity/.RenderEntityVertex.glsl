#version 450
#extension GL_ARB_separate_shader_objects : enable

// Camera-based unform - changes each frame based on the camera's POV and chunk data
layout(set = 0, binding = 0) uniform CameraUniform {
	mat4 view;
	mat4 proj;
	vec3 posOfCurrentChunk;
	vec3 chunkSize;
} camera;

// Model attributes - changes based on the block type being drawn
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texCoord;

// Instance attributes - changes based on a specific block being drawn
layout(location = 5) in vec3 posOfCurrentChunk;
layout(location = 6) in mat4 modelMatrix; // slots [6,10)

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	vec3 chunkPosOffset = (posOfCurrentChunk - camera.posOfCurrentChunk) * camera.chunkSize; // component-wise multiple to convert chunk pos to world pos
	mat4 chunkAdjustmentMatrix = mat4(1);
	chunkAdjustmentMatrix[3] = vec4(chunkPosOffset, 1.0);

	gl_Position =
		camera.proj
		* camera.view
		* chunkAdjustmentMatrix
		* modelMatrix * vec4(position, 1.0);
	fragColor = vec4(1);
	fragTexCoord = texCoord;
}
