#type vertex
#version 450


layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec3 a_Color;

layout(location = 0) out vec3 v_col;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 ViewProj;
} ubo;


void main()
{
	gl_Position = ubo.ViewProj * ubo.Model * vec4(a_Position, 0.0f, 1.0f);
	v_col = a_Color;
}

#type pixel
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 v_col;


void main()
{
	outColor = vec4(v_col, 1.0f);
}