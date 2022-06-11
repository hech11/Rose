#type vertex
#version 450


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;


struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
	mat3 WorldNormals;
	mat3 WorldTransform;

	mat3 CameraView;
	vec3 ViewPosition;
};

layout(location = 0) out VertexOutput v_Output;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
} ubo;


void main()
{

	vec4 worldPos = ubo.Model * vec4(a_Position, 1.0);
	v_Output.WorldPosition = worldPos.xyz;
	v_Output.Normal = mat3(ubo.Model) * a_Normal;
	v_Output.TexCoord = a_TexCoord;
	v_Output.Tangent = a_Tangent;


	mat3 nMatrix = transpose(inverse(mat3(ubo.Model)));

	vec3 T = normalize(nMatrix * a_Tangent.xyz);
	vec3 N = normalize(nMatrix * a_Normal.xyz);

	vec3 B = normalize(cross(N, T)* 1.0f);

	v_Output.WorldNormals = mat3(T, B, N);
	v_Output.WorldTransform = mat3(ubo.Model);
	v_Output.CameraView = mat3(ubo.View);


	v_Output.ViewPosition = vec3(ubo.View * worldPos);

	gl_Position = ubo.ViewProj * worldPos;
}

#type pixel
#version 450


layout(set = 0, binding = 1) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_NormalMap;


struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
	mat3 WorldNormals;
	mat3 WorldTransform;

	mat3 CameraView;
	vec3 ViewPosition;
};
layout(location = 0) in VertexOutput v_Input;


layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 NormalMap()
{
	vec3 tangentNormal = texture(u_NormalMap, v_Input.TexCoord).xyz * 2.0 - 1.0;
	return normalize(v_Input.WorldNormals * tangentNormal);
}

void main()
{


	vec3 albedo = pow(texture(u_AlbedoMap, v_Input.TexCoord).rgb, vec3(2.2));
	float metallic = 0.1f;
	float roughness = 0.8f;
	float ao = 1.0f;

	vec3 LightColor = vec3(50.0, 50.0, 50.0);

	vec3 WorldPos = v_Input.WorldPosition;
	vec3 LightPos = { 0.0f, 20.0f, 0.0f };

	vec3 N = NormalMap();
	vec3 V = normalize(v_Input.ViewPosition - WorldPos);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);


	vec3 L = normalize(LightPos - WorldPos);

	float cosTheta = max(dot(N, L), 0.0);
	vec3 H = normalize(V + L);
	float distance = length(LightPos - WorldPos);
	float radius = 250.0;

	float attenuation = clamp(1.0 - (distance * distance) / (radius* radius), 0.0, 1.0);
	attenuation *= mix(attenuation, 1.0, 1.0); // falloff

	vec3 radiance = LightColor * attenuation * cosTheta;



	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);


	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	float ambientStrength = 0.03f;
	vec3 ambient = ambientStrength * albedo;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

//	if (color.a < 0.8)
		//discard;
	fragColor = vec4(color, 1.0f);
}