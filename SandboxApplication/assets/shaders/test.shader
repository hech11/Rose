#type vertex
#version 450

vec2 pos[3] = vec2[](
	vec2(0.0f, -0.5f),
	vec2(0.5f,  0.5f),
	vec2(-0.5f, 0.5f)
);


vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out vec3 v_col;



void main()
{
	gl_Position = vec4(pos[gl_VertexIndex], 0.0f, 1.0f);
	v_col = colors[gl_VertexIndex];
}

#type pixel
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 v_col;


void main()
{
	outColor = vec4(v_col, 1.0f);
}