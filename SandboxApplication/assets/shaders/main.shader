#type vertex
#version 450


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;


layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 3) out vec3 v_FragPos;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 ViewProj;
} ubo;


void main()
{
	gl_Position = ubo.ViewProj * ubo.Model * vec4(a_Position, 1.0f);
	v_FragPos = vec3(ubo.Model * vec4(a_Position, 1.0f));
	
	v_Normal = a_Normal;
	v_TexCoord = a_TexCoord;
}

#type pixel
#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 3) in vec3 v_FragPos;


layout(binding = 1) uniform sampler2D u_Texture;

void main()
{
	float ambientStrength = 0.2;
	vec3 ambient = ambientStrength * vec3(1.0f, 1.0f, 1.0f);

	vec3 lightPos = { 3.0f, 20.0f, 0.0f };

	vec3 norm = normalize(v_Normal);
	vec3 lightDir = normalize(lightPos - v_FragPos);


	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = vec3(diff, diff, diff);

	vec4 color = texture(u_Texture, v_TexCoord);
	if (color.a < 0.8)
		discard;
	fragColor = color;
}