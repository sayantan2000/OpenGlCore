#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 UV;

// ---- Material (metallic-roughness) ----
uniform vec3  u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;

uniform bool u_hasAlbedoMap;    uniform sampler2D u_albedoMap;
uniform bool u_hasNormalMap;    uniform sampler2D u_normalMap;
uniform bool u_hasMetallicMap;  uniform sampler2D u_metallicMap;
uniform bool u_hasRoughnessMap; uniform sampler2D u_roughnessMap;
uniform bool u_hasAoMap;        uniform sampler2D u_aoMap;

// ---- Camera ----
uniform vec3 u_camPos;

// ---- Lights ----
struct DirLight   { vec3 direction; vec3 color; };
struct PointLight { vec3 position;  vec3 color; };
struct SpotLight  { vec3 position;  vec3 direction; vec3 color; float innerCos; float outerCos; };
// Area light: a rectangle centred at `position`, emitting along +normal, spanning
// +/-halfWidth along `right` and +/-halfHeight along `up`.
struct RectLight  { vec3 position; vec3 normal; vec3 right; vec3 up; vec3 color; float halfWidth; float halfHeight; };

#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS  4
#define MAX_RECT_LIGHTS  4

uniform bool       u_hasDirLight;
uniform DirLight   u_dirLight;
uniform int        u_numPointLights;
uniform PointLight u_pointLights[MAX_POINT_LIGHTS];
uniform int        u_numSpotLights;
uniform SpotLight  u_spotLights[MAX_SPOT_LIGHTS];
uniform int        u_numRectLights;
uniform RectLight  u_rectLights[MAX_RECT_LIGHTS];

// ---- IBL ----
uniform bool u_useIBL;
uniform samplerCube u_irradianceMap;   // unit 0
uniform samplerCube u_prefilterMap;    // unit 1
uniform sampler2D   u_brdfLUT;         // unit 2
uniform float u_maxReflectionLod;      // prefilter mip count - 1

const float PI = 3.14159265359;

// Perturb the geometric normal with a tangent-space normal map using screen-space
// derivatives (no precomputed tangents needed).
vec3 getNormalFromMap()
{
	if (!u_hasNormalMap)
		return normalize(Normal);

	vec3 tangentNormal = texture(u_normalMap, UV).xyz * 2.0 - 1.0;

	vec3 Q1  = dFdx(WorldPos);
	vec3 Q2  = dFdy(WorldPos);
	vec2 st1 = dFdx(UV);
	vec2 st2 = dFdy(UV);

	vec3 N   = normalize(Normal);
	vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B   = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;
	return a2 / max(denom, 1e-7);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Cook-Torrance contribution of one light with radiance L_color arriving along L.
vec3 shade(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness, vec3 F0)
{
	vec3 H = normalize(V + L);
	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 numerator = NDF * G * F;
	float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
	vec3 specular = numerator / denom;

	vec3 kS = F;
	vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic); // energy conservation
	float NdotL = max(dot(N, L), 0.0);
	return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Rectangle (area) light via a representative-point approximation (not full LTC): the
// diffuse term uses the closest point on the rectangle to the shading point, and the
// specular term uses the point where the reflection ray meets the rectangle (clamped to
// its extents). Flux scales with the emitter area and one-sided emission along +normal.
vec3 shadeRect(vec3 N, vec3 V, RectLight rl, vec3 albedo, float metallic, float roughness, vec3 F0)
{
	vec3 toFrag = WorldPos - rl.position;
	float facing = dot(rl.normal, normalize(toFrag)); // >0: fragment is in front of the emitter
	if (facing <= 0.0) return vec3(0.0);

	// Closest point on the rectangle (for the diffuse lobe).
	float pr = clamp(dot(toFrag, rl.right), -rl.halfWidth,  rl.halfWidth);
	float pu = clamp(dot(toFrag, rl.up),    -rl.halfHeight, rl.halfHeight);
	vec3 closest = rl.position + rl.right * pr + rl.up * pu;
	vec3 Ld = closest - WorldPos;
	float distD = max(length(Ld), 1e-4);
	Ld /= distD;

	float area = (2.0 * rl.halfWidth) * (2.0 * rl.halfHeight);
	float atten = area / max(distD * distD, 1e-4);
	vec3 radiance = rl.color * atten * facing;

	// Diffuse (Lambert) with the closest-point direction.
	float NdotLd = max(dot(N, Ld), 0.0);
	vec3 kD = (1.0 - metallic) * albedo / PI;
	vec3 diffuse = kD * radiance * NdotLd;

	// Specular representative point: reflection ray vs the light plane, clamped to extents.
	vec3 R = reflect(-V, N);
	vec3 Ls = Ld;
	float denom = dot(R, -rl.normal);
	if (denom > 1e-4)
	{
		float t = dot(rl.position - WorldPos, -rl.normal) / denom;
		if (t > 0.0)
		{
			vec3 hit = WorldPos + R * t - rl.position;
			float hr = clamp(dot(hit, rl.right), -rl.halfWidth,  rl.halfWidth);
			float hu = clamp(dot(hit, rl.up),    -rl.halfHeight, rl.halfHeight);
			Ls = normalize(rl.position + rl.right * hr + rl.up * hu - WorldPos);
		}
	}
	vec3 H = normalize(V + Ls);
	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, Ls, roughness);
	vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
	float specDenom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, Ls), 0.0) + 1e-4;
	vec3 specular = (NDF * G * F) / specDenom * radiance * max(dot(N, Ls), 0.0);

	return diffuse + specular;
}

void main()
{
	// Sample material (albedo map assumed sRGB, converted to linear).
	vec3 albedo = u_hasAlbedoMap ? pow(texture(u_albedoMap, UV).rgb, vec3(2.2)) : u_albedo;
	float metallic  = u_hasMetallicMap  ? texture(u_metallicMap, UV).r  : u_metallic;
	float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, UV).r : u_roughness;
	float ao        = u_hasAoMap        ? texture(u_aoMap, UV).r        : u_ao;

	vec3 N = getNormalFromMap();
	vec3 V = normalize(u_camPos - WorldPos);
	vec3 R = reflect(-V, N);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	vec3 Lo = vec3(0.0);

	// Directional
	if (u_hasDirLight)
	{
		vec3 L = normalize(-u_dirLight.direction);
		Lo += shade(N, V, L, u_dirLight.color, albedo, metallic, roughness, F0);
	}

	// Point lights (inverse-square falloff)
	for (int i = 0; i < u_numPointLights; ++i)
	{
		vec3 d = u_pointLights[i].position - WorldPos;
		float dist = length(d);
		vec3 L = d / max(dist, 1e-4);
		float attenuation = 1.0 / max(dist * dist, 1e-4);
		Lo += shade(N, V, L, u_pointLights[i].color * attenuation, albedo, metallic, roughness, F0);
	}

	// Spot lights (inverse-square + smooth cone)
	for (int i = 0; i < u_numSpotLights; ++i)
	{
		vec3 d = u_spotLights[i].position - WorldPos;
		float dist = length(d);
		vec3 L = d / max(dist, 1e-4);
		float attenuation = 1.0 / max(dist * dist, 1e-4);
		float theta = dot(normalize(-L), normalize(u_spotLights[i].direction));
		float epsilon = u_spotLights[i].innerCos - u_spotLights[i].outerCos;
		float cone = clamp((theta - u_spotLights[i].outerCos) / max(epsilon, 1e-4), 0.0, 1.0);
		Lo += shade(N, V, L, u_spotLights[i].color * attenuation * cone, albedo, metallic, roughness, F0);
	}

	// Rectangle / area lights (representative-point approximation)
	for (int i = 0; i < u_numRectLights; ++i)
	{
		Lo += shadeRect(N, V, u_rectLights[i], albedo, metallic, roughness, F0);
	}

	// Ambient / IBL
	vec3 ambient;
	if (u_useIBL)
	{
		vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
		vec3 kS = F;
		vec3 kD = (1.0 - kS) * (1.0 - metallic);

		vec3 irradiance = texture(u_irradianceMap, N).rgb;
		vec3 diffuse = irradiance * albedo;

		vec3 prefiltered = textureLod(u_prefilterMap, R, roughness * u_maxReflectionLod).rgb;
		vec2 brdf = texture(u_brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
		vec3 specular = prefiltered * (F * brdf.x + brdf.y);

		ambient = (kD * diffuse + specular) * ao;
	}
	else
	{
		ambient = vec3(0.03) * albedo * ao;
	}

	FragColor = vec4(ambient + Lo, 1.0); // HDR linear output; tonemapped in a later pass
}
