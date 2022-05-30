#type vertex
#version 450

vec2 pos[3] = vec2[](
	vec2(0.0f, -0.5f),
	vec2(0.5f,  0.5f),
	vec2(-0.5f, 0.5f)
);

void main()
{
	gl_Position = vec4(pos[gl_VertexIndex], 0.0f, 1.0f);
}

#type pixel
#version 450

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}