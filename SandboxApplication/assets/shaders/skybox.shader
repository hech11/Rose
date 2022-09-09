#type vertex
#version 450


layout(location = 0) in vec3 a_Position;

layout(std140, binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;


	vec4 DirectionLightDir;
	vec4 DirectionLightColor;

	vec4 DirLightIntensity;
	vec4 EnivormentMapIntensity;
} ubo;


struct VertexOutput
{
	vec3 LocalPosition;
	vec3 TexCoord;

	float EnvIntensity;
};

layout(location = 0) out VertexOutput v_Output;

void main()
{

	mat4 rot = mat4(transpose(mat3(ubo.View)));

	vec4 worldPos = ubo.Proj * rot * vec4(a_Position, 1.0);
	gl_Position = worldPos.xyww;

	v_Output.LocalPosition = a_Position;
	v_Output.EnvIntensity = ubo.EnivormentMapIntensity.x;
	v_Output.TexCoord = vec3(a_Position.x, a_Position.y, -a_Position.z);
}

#type pixel
#version 450

layout(set = 0, binding = 1) uniform samplerCube u_AlbedoMap;

layout(location = 0) out vec4 fragColor;



struct VertexOutput
{
	vec3 LocalPosition;
	vec3 TexCoord;

	float EnvIntensity;
};

layout(location = 0) in VertexOutput v_Input;


void main()
{

	vec3 color = pow(texture(u_AlbedoMap, v_Input.TexCoord).rgb, vec3(2.2));

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	fragColor = vec4(color * v_Input.EnvIntensity, 1.0f);
}