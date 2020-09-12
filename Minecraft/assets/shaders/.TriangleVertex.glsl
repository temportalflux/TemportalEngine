#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform ModelViewProjection {
	mat4 view;
	mat4 proj;
	vec3 chunkPos;
	vec3 chunkSize;
} mvp;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;

layout(location = 3) in vec3 posInChunk;
layout(location = 4) in mat4 modelMatrix; // slots [4,7]

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	vec3 blockPosRelativeToCameraChunk = (posInChunk - mvp.chunkPos) * mvp.chunkSize; // component-wise multiple to convert chunk pos to world pos
	vec3 blockPos = blockPosRelativeToCameraChunk + position;
	//vec3 blockPos = position;
	gl_Position = mvp.proj * mvp.view * modelMatrix * vec4(blockPos, 1.0);
	fragColor = color;
	fragTexCoord = texCoord;
}
