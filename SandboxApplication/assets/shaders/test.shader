#type vertex
#version 450


layout(location = 0) in vec3 a_Position;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 ViewProj;
} ubo;


void main()
{
	gl_Position = ubo.ViewProj * ubo.Model * vec4(a_Position, 1.0f);
	
}

#type pixel
#version 450

layout(location = 0) out vec4 fragColor;



void main()
{
	vec4 color = vec4(1.0f);
	fragColor = color;
}