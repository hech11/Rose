#type vertex
#version 450


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Bitangent;
layout(location = 4) in vec2 a_TexCoord;


struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
	vec3 Bitangent;
	mat3 WorldNormals;
	mat3 WorldTransform;

	mat3 CameraView;
	vec3 ViewPosition;

	mat4 Model;
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
	v_Output.Bitangent = a_Bitangent;


	v_Output.Model = ubo.Model;

	mat3 nMatrix = transpose(inverse(mat3(ubo.Model)));

	vec3 T = normalize(nMatrix * a_Bitangent.xyz);
	vec3 N = normalize(nMatrix * a_Normal.xyz);

	vec3 B = normalize(cross(N, T)* 1.0f);

	v_Output.WorldNormals = mat3(T, B, N);
	v_Output.WorldTransform = mat3(ubo.Model);
	v_Output.CameraView = mat3(ubo.View);


	v_Output.ViewPosition = vec3(ubo.View * vec4(v_Output.WorldPosition, 1.0f));

	gl_Position = ubo.ViewProj * worldPos;
}

#type pixel
#version 450


layout(set = 0, binding = 1) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_NormalMap;




struct DirectionLight
{
	vec3 Direction;
	vec3 Radience;
};

struct PointLight
{
	vec3 Position;
	vec3 Radience;
	float Radius;
};


struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
	vec3 Bitangent;

	mat3 WorldNormals;
	mat3 WorldTransform;

	mat3 CameraView;
	vec3 ViewPosition;

	mat4 Model;

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


vec3 CalcDirectionLight(DirectionLight light, vec3 F0, vec3 albedo, vec3 N, float metallic, float roughness, float ao, vec3 worldPos)
{

	vec3 LightColor = light.Radience;

	vec3 WorldPos = worldPos;

	vec3 V = normalize(v_Input.ViewPosition - worldPos);


	vec3 Lo = vec3(0.0f);

	vec3 L = light.Direction;

	float cosTheta = max(dot(N, L), 0.0);
	vec3 H = normalize(V + L);


	vec3 radiance = LightColor * cosTheta;


	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);


	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	return Lo;
}


vec3 CalcPointLight(PointLight light, vec3 F0, vec3 albedo, vec3 N, float metallic, float roughness, float ao, vec3 worldPos)
{
	
	vec3 LightColor = light.Radience;

	vec3 WorldPos = worldPos;
	vec3 LightPos = light.Position;

	vec3 V = normalize(v_Input.ViewPosition - worldPos);


	vec3 Lo = vec3(0.0);

	vec3 L = normalize(LightPos - WorldPos);

	float cosTheta = max(dot(N, L), 0.0);
	vec3 H = normalize(V+L);
	float distance = length(LightPos - WorldPos);
	float radius = light.Radius;

	float attenuation = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
	attenuation *= mix(attenuation, 1.0, 1.0); // falloff

	vec3 radiance = LightColor * attenuation * cosTheta;


	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);


	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	return Lo;
}

void main()
{

	vec3 albedo = pow(texture(u_AlbedoMap, v_Input.TexCoord).rgb, vec3(2.2));
	float metallic = 0.2f;
	float roughness = 1.0f;
	float ao = 0.1f;

	vec3 NN = NormalMap();

	
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 color = vec3(0.1f);


	DirectionLight dirLight;
	dirLight.Direction = vec3(0.5f, 1.0f, 0.5f);
	dirLight.Radience = vec3(245.0f/ 256.0f, 66.0f / 256.0f, 90.0f / 256.0f)*2.5f;

	PointLight light1;
	light1.Position = vec3(0.0f, 60.0f, 0.0f);
	light1.Radience = vec3(15.0f, 15.0f, 12.0f);
	light1.Radius = 250.0f;


	/*mat3 nMatrix = transpose(inverse(mat3(v_Input.Model)));

	vec3 T = normalize(nMatrix * v_Input.Tangent.xyz);
	vec3 N = normalize(nMatrix * v_Input.Normal.xyz);

	vec3 B = normalize(cross(T, N));

	mat3 wNormal = mat3(T, B, N);
	vec3 tangentNormal = texture(u_NormalMap, v_Input.TexCoord).xyz * 2.0 - 1.0;

	NN = wNormal * tangentNormal;
	*/
	color = CalcDirectionLight(dirLight, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);
	color += CalcPointLight(light1, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);


	PointLight light2;
	light2.Position = vec3(150.0f*2, 20.0f, 0.0f);
	light2.Radience = vec3(15.0f, 15.0f, 25.0f);
	light2.Radius = 150.0f;

	color += CalcPointLight(light2, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);



	float ambientStrength = 0.02f;
	vec3 ambient = ambientStrength * albedo;
	color += ambient;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));


//	if (color.a < 0.8)
		//discard;
	fragColor = vec4(color, 1.0f);
}