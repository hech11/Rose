#type vertex
#version 450


layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_Col;
layout(location = 1) out vec2 v_TexCoord;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 ViewProj;
} ubo;


void main()
{
	gl_Position = ubo.ViewProj * ubo.Model * vec4(a_Position, 0.0f, 1.0f);
	v_Col = a_Color;
	v_TexCoord = a_TexCoord;
	
}

#type pixel
#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 v_Col;
layout(location = 1) in vec2 v_TexCoord;


layout(binding = 1) uniform sampler2D u_Texture;

void main()
{
	vec4 color = texture(u_Texture, v_TexCoord);
	fragColor = color;
}