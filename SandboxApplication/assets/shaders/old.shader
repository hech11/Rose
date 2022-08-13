#type vertex
#version 450


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;


layout(location = 0) out vec3 v_Normal;
layout(location = 1) out mat3 v_TBN;
layout(location = 4) out vec2 v_TexCoord;
layout(location = 5) out vec3 v_FragPos;


layout(binding = 0) uniform BufferObject
{
	mat4 Model;
	mat4 ViewProj;
} ubo;


void main()
{

	mat3 nMatrix = transpose(inverse(mat3(ubo.Model)));

	vec3 T = normalize(nMatrix * a_Tangent.xyz);
	vec3 N = normalize(nMatrix * a_Normal.xyz);

	vec3 B = normalize(cross(N, T) * 1.0f);


	gl_Position = ubo.ViewProj * ubo.Model * vec4(a_Position, 1.0f);
	v_FragPos = vec3(ubo.Model * vec4(a_Position, 1.0f));

	v_Normal = normalize(nMatrix * a_Normal);
	v_TexCoord = a_TexCoord;
	v_TBN = mat3(T, B, N);
}

#type pixel
#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in mat3 v_TBN;
layout(location = 4) in vec2 v_TexCoord;
layout(location = 5) in vec3 v_FragPos;


layout(set = 0, binding = 1) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_NormalMap;


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


void main()
{


	vec3 albedoColor = vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 0.1f;
	float ao = 0.0f;

	vec3 WorldPos = vec3(0.0f, 0.0f, -10.0f);
	vec3 LightPos = { 3.0f, 20.0f, 0.0f };
	vec3 N = normalize(v_Normal);
	vec3 V = normalize(vec3(0.0f, 0.0f, -10.0f) - WorldPos);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);


	vec3 L = normalize(LightPos - WorldPos);
	vec3 H = normalize(V + L);
	float distance = length(LightPos - WorldPos);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = vec4(1.0f) * attenuation;

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	float ambientStrength = 0.03;
	vec3 ambient = ambientStrength * albedo * ao;


	vec3 normalData = texture(u_NormalMap, v_TexCoord).rgb;
	vec3 norm = v_TBN * (normalData * 2.0f - 1.0f);

	vec3 lightDir = normalize(lightPos - v_FragPos);


	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = vec3(diff, diff, diff);

	vec4 color = texture(u_AlbedoMap, v_TexCoord);

	color = color * vec4(diffuse + ambient, 1.0f);
	if (color.a < 0.8)
		discard;
	fragColor = color;
}