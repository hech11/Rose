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


const float PI = 3.14159265359;
const vec2 invAtan = vec2(0.1591, 0.3183);


vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main()
{

	//vec2 uv = SampleSphericalMap(normalize(v_Input.LocalPosition));

	vec3 color = pow(texture(u_AlbedoMap, v_Input.TexCoord).rgb, vec3(2.2));
	//vec3 albedo = texture(u_AlbedoMap, uv).rgb;


	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));
	

	//	if (color.a < 0.8)
			//discard;
	fragColor = vec4(color * v_Input.EnvIntensity, 1.0f);
}