#type vertex
#version 450 core


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;


struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
	vec3 Binormal;
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
	mat4 transform = ubo.Model;


	vec4 worldPos = vec4(a_Position, 1.0);

	v_Output.WorldPosition = worldPos.xyz;
	v_Output.Normal = mat3(transform) * a_Normal;

	v_Output.TexCoord = a_TexCoord;
	v_Output.Tangent = a_Tangent;
	mat3 nMatrix = mat3(ubo.Model);

	vec3 T = normalize(nMatrix * a_Binormal.xyz);
	vec3 N = normalize(nMatrix * a_Normal.xyz);
	vec3 B = normalize(cross(N, T) * 1.0f);

	v_Output.Binormal = B;

	v_Output.WorldTransform = mat3(transform);
	v_Output.WorldNormals = mat3(transform) * mat3(a_Tangent, B, a_Normal);

	v_Output.CameraView = mat3(ubo.View);


	mat4 inverseView = ubo.View;

	vec3 pos = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
	//v_Output.ViewPosition = vec3(ubo.View * vec4(v_Output.WorldPosition, 1.0f));
	v_Output.ViewPosition = pos;

	gl_Position = ubo.ViewProj * worldPos;
}

#type pixel
#version 450 core


layout(set = 0, binding = 1) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 3) uniform samplerCube u_IrradienceMap;
layout(set = 0, binding = 4) uniform samplerCube u_RadienceMap;
layout(set = 0, binding = 5) uniform sampler2D u_SpecularBRDFLUTTexture;



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
	vec3 Binormal;

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

float GaSchlick1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

float GaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return GaSchlick1(cosLi, k) * GaSchlick1(NdotV, k);
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

float NdfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

vec3 fresnelSchlickRough(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 NormalMap()
{
	vec3 tangentNormal = normalize(v_Input.Normal);
	tangentNormal = normalize(texture(u_NormalMap, v_Input.TexCoord).xyz * 2.0 - 1.0);
	return normalize(v_Input.WorldNormals * tangentNormal);
}


vec3 CalcDirectionLight(DirectionLight light, vec3 F0, vec3 albedo, vec3 N, float metallic, float roughness, float ao, vec3 worldPos)
{
	vec3 view = normalize(v_Input.ViewPosition - v_Input.WorldPosition);
	float NdotV = max(dot(N, view), 0.0);

	vec3 LightColor = light.Radience;

	vec3 WorldPos = worldPos;
	vec3 Direction = light.Direction;

	vec3 L = Direction;
	vec3 Lh = normalize(L + view);


	vec3 radiance = LightColor;


	float cosLi = max(0.0, dot(N, L));
	float cosLh = max(0.0, dot(N, Lh));


	vec3 F = fresnelSchlickRough(F0, max(0.0, dot(Lh, view)), roughness);
	float D = NdfGGX(cosLh, roughness);
	float G = GaSchlickGGX(cosLi, NdotV, roughness);

	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 result = vec3(0.0f);
	vec3 diffuse = kD * albedo;

	vec3 spec = (F * D * G) / max(0.00001, 4.0f * cosLi * NdotV);
	spec = clamp(spec, vec3(0.0f), vec3(10.0f));


	result = (diffuse + spec) * radiance * cosLi;
	return result;
}
float Convert_sRGB_FromLinear(float theLinearValue)
{
	return theLinearValue <= 0.0031308f
		? theLinearValue * 12.92f
		: pow(theLinearValue, 1.0f / 2.4f) * 1.055f - 0.055f;
}

vec3 CalcPointLight(PointLight light, vec3 F0, vec3 albedo, vec3 N, float metallic, float roughness, float ao, vec3 worldPos)
{
	vec3 view = normalize(v_Input.ViewPosition - v_Input.WorldPosition);
	float NdotV = max(dot(N, view), 0.0);

	vec3 LightColor = light.Radience;

	vec3 WorldPos = worldPos;
	vec3 LightPos = light.Position;

	vec3 L = normalize(LightPos - WorldPos);
	vec3 Lh = normalize(L + view);

	float distance = length(LightPos - WorldPos);
	float radius = light.Radius;

	float attenuation = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
	attenuation *= mix(attenuation, 1.0, 1.0); // falloff

	vec3 radiance = LightColor* attenuation;


	float cosLi = max(0.0, dot(N, L));
	float cosLh = max(0.0, dot(N, Lh));


	vec3 F = fresnelSchlickRough(F0, max(0.0, dot(Lh, view)), roughness);
	float D = NdfGGX(cosLh, roughness);
	float G = GaSchlickGGX(cosLi, NdotV, roughness);

	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 result = vec3(0.0f);
	vec3 diffuse = kD * albedo;

	vec3 spec = (F * D * G) / max(0.00001, 4.0f * cosLi * NdotV);
	spec = clamp(spec, vec3(0.0f), vec3(10.0f));


	result = (diffuse + spec) * radiance * cosLi;
	//result = vec3(NdotV, NdotV, NdotV);
	return result;
}


vec3 IBL(vec3 F0,vec3 NN, vec3 albedo, float metal, float rough)
{


	vec3 view = normalize(v_Input.ViewPosition - v_Input.WorldPosition);
	float NdotV = max(dot(NN, view), 0.0);

	vec3 F = fresnelSchlickRough(F0, NdotV, rough);
	vec3 kd = (1.0 - F) * (1.0 - metal);



	vec3 irradiance = texture(u_IrradienceMap, NN).rgb;

	vec3 diffuseIBL = albedo *irradiance;


	vec3 Lr = 2.0 * NdotV * NN - view;
	float angle = radians(0.0f);
	mat3x3 rotMat = { vec3(cos(angle),0.0,sin(angle)), vec3(0.0,1.0,0.0), vec3(-sin(angle),0.0,cos(angle)) };
	vec3 rotY = rotMat * Lr;

	int envRadianceTexLevels = textureQueryLevels(u_RadienceMap);

	vec3 specularIrradiance = textureLod(u_RadienceMap, rotY, rough).xyz;
	specularIrradiance = vec3(Convert_sRGB_FromLinear(specularIrradiance.x), Convert_sRGB_FromLinear(specularIrradiance.y), Convert_sRGB_FromLinear(specularIrradiance.z));
	vec2 specularBRDF = texture(u_SpecularBRDFLUTTexture, vec2(NdotV, 1.0 - rough)).xy;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);


	return kd* diffuseIBL + specularIBL;
}

void main()
{

	vec4 albedoSample = texture(u_AlbedoMap, v_Input.TexCoord);

	float alpha = albedoSample.a;
	vec3 albedo = pow(albedoSample.rgb, vec3(2.2));

	float metallic = 0.1f;
	float roughness = 0.8f;
	float ao = 0.1f;

	vec3 NN = NormalMap();
	
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 color = vec3(0.1f);


	DirectionLight dirLight;
	dirLight.Direction = vec3(0.5f, 1.0f, 0.5f);
	dirLight.Radience = vec3(255.0f/ 256.0f, 254.0f / 256.0f, 230.0f / 256.0f)*1.5f;

	PointLight light1;
	light1.Position = vec3(0.0f, 60.0f, 0.0f);
	light1.Radience = vec3(15.0f, 15.0f, 12.0f);
	light1.Radius = 250.0f;

	
	
	vec3 lightContribution = vec3(0.1f);
	lightContribution = CalcDirectionLight(dirLight, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);
	lightContribution += CalcPointLight(light1, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);


	PointLight light2;
	light2.Position = vec3(150.0f*2, 20.0f, 0.0f);
	light2.Radience = vec3(15.0f, 15.0f, 25.0f);
	light2.Radius = 150.0f;


	lightContribution += CalcPointLight(light2, F0, albedo, NN, metallic, roughness, ao, v_Input.WorldPosition);

	vec3 IBLContribution = vec3(0.0f);
	IBLContribution = IBL(F0, NN, albedo, metallic, roughness);
	//color = v_Input.WorldNormals * vec3(1.0f);
	//color = v_Input.Tangent;
	//color = v_Input.Binormal;
	//color = v_Input.Normal;
	//color = v_Input.WorldPosition;
	//color = vec3(1.0f);
	color = IBLContribution+lightContribution;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));


	if (alpha < 0.8)
		discard;
	fragColor = vec4(color, alpha);
}