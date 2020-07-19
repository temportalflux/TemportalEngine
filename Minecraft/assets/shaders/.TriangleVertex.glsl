#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform ModelViewProjection {
	mat4 view;
	mat4 proj;
} mvp;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 modelMatrix; // slots [3,6]

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	gl_Position = mvp.proj * mvp.view * modelMatrix * vec4(position, 1.0);
	fragColor = color;
	fragTexCoord = texCoord;
}
