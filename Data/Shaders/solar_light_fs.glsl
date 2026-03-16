#extension GL_EXT_texture_array : require

#pragma include "misc.glsl"

#pragma include "atmosphere_utils.glsl"

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_color;

uniform vec3 uCameraLocalLightPos;
uniform vec3 uLightColor;

#define MAX_ATMOSPHERES 16
uniform vec3	uPlanetCenter[MAX_ATMOSPHERES];
uniform float	uInnerRadius[MAX_ATMOSPHERES];
uniform float	uOneOverAtmosphereDepth[MAX_ATMOSPHERES];
uniform vec3	uBeta[MAX_ATMOSPHERES];
uniform int		uSurfaceLightingTextureIndex[MAX_ATMOSPHERES];
uniform int		uOpticalDepthRayleighTextureIndex[MAX_ATMOSPHERES];
uniform int		uOpticalDepthMieTextureIndex[MAX_ATMOSPHERES];

uniform sampler2DArray	uSurfaceLightingTexture;
uniform sampler2DArray	uOpticalDepthRayleighTexture;
uniform sampler2DArray	uOpticalDepthMieTexture;

uniform sampler2DRect	uNormalBufferTexture;
uniform sampler2DRect 	uAlbedoBufferTexture;
uniform sampler2DRect	uPositionBufferTexture;

uniform sampler2DArrayShadow uShadowDepthTextures;


#define MAX_FRUSTUMS 16
uniform int		uFrustumCount;
uniform float	uFarDist[MAX_FRUSTUMS];
uniform mat4	uLightFrustum[MAX_FRUSTUMS];

float shadow_coeff(in float depth, in vec3 vPos)
{
	if(depth > uFarDist[uFrustumCount-1])
	{
		return 1.0;
	}

	int index = 0;
	// find the appropriate depth map to look up in based on the depth of this fragment
	for(int i = uFrustumCount - 2; i >= 0; --i)
	{
		if(depth > uFarDist[i])
		{
			index = i + 1;
			break;
		}
	}
	float fade = 0;
	if(index == uFrustumCount - 1)
	{
		fade = (depth - uFarDist[uFrustumCount - 2]) / (uFarDist[uFrustumCount - 1] - uFarDist[uFrustumCount - 2]);
	}

	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadowCoord = uLightFrustum[index] * vec4(vPos, 1);
	shadowCoord.w = shadowCoord.z;
	
	// tell glsl in which layer to do the look up
	shadowCoord.z = float(index);
	
	// return the shadow contribution
	float texDepth = shadow2DArray(uShadowDepthTextures, shadowCoord).x * 0.25;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, -1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, 0)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, 1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 0, -1)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 0, 1)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, -1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, 0)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, 1)).x * 0.0625;
	return min(1, texDepth + fade);
}

vec3 shadow_coeff_test(in float depth, in vec3 vPos)
{
 	if(depth > uFarDist[uFrustumCount-1])
 	{
 		return vec3(0.5);
 	}

	int index = 0;
	// find the appropriate depth map to look up in based on the depth of this fragment
	for(int i = uFrustumCount - 2; i >= 0; --i)
	{
		if(depth > uFarDist[i])
		{
			index = i + 1;
			break;
		}
	}
	if(index == 0)
		return vec3(1, 0, 0);
	if(index == 1)
		return vec3(0, 1, 0);
	if(index == 2)
		return vec3(0, 0, 1);
	return vec3(1, 1, 0);

	float fade = 0;
	if(index == uFrustumCount - 2)
	{
		fade = (depth - uFarDist[uFrustumCount - 2]) / (uFarDist[uFrustumCount - 1] - uFarDist[uFrustumCount - 2]);
	}

	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadowCoord = uLightFrustum[index] * vec4(vPos, 1);
	shadowCoord.w = shadowCoord.z;
	
	// tell glsl in which layer to do the look up
	shadowCoord.z = float(index);
	
	// return the shadow contribution
	float texDepth = shadow2DArray(uShadowDepthTextures, shadowCoord).x * 0.25;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, -1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, 0)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( -1, 1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 0, -1)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 0, 1)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, -1)).x * 0.0625;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, 0)).x * 0.125;
	texDepth += shadow2DArrayOffset(uShadowDepthTextures, shadowCoord, ivec2( 1, 1)).x * 0.0625;
	return vec3(1 /*min(1, texDepth + fade)*/, 50 + index * 50, 0);
}

vec3 get_Isky_and_optical_depth(float sunAngle, float height, int planetID, out vec2 opticalDepth)
{
 	float xv = height * uOneOverAtmosphereDepth[planetID];
 	float yv = sunAngle * AngleRange;
  	opticalDepth.x = texture(uOpticalDepthRayleighTexture, vec3(xv, yv, uOpticalDepthRayleighTextureIndex[planetID])).r;
	opticalDepth.y = texture(uOpticalDepthMieTexture, vec3(xv, yv, uOpticalDepthMieTextureIndex[planetID])).r;
  	return texture(uSurfaceLightingTexture, vec3(xv, yv, uSurfaceLightingTextureIndex[planetID])).rgb;
}

void main()
{
 	const float ambientFactor = 0.5;
 

 	vec4 cameraCoordAndClipZ = texture(uPositionBufferTexture, In.uv);
	//out_color = vec4(shadow_coeff_test(cameraCoordAndClipZ.w, cameraCoordAndClipZ.xyz), 1);
	//return ;
 	float shadowCoeff = shadow_coeff(cameraCoordAndClipZ.w, cameraCoordAndClipZ.xyz);
 
	vec3 lightVec = uCameraLocalLightPos - cameraCoordAndClipZ.xyz;
	float lightDistance = length(lightVec);
	vec3 lightDir = lightVec / lightDistance;
 	vec3 normal = texture(uNormalBufferTexture, In.uv).xyz;
 	float diffuseCoeff = max(0, dot(normal, lightDir)) * 0.2;
 
  	vec4 albedoAndPlanetID = texture(uAlbedoBufferTexture, In.uv);
	vec3 albedo = albedoAndPlanetID.xyz;
	int planetID = int(albedoAndPlanetID.w);

	float lightIntesity = calculate_intensity(lightDistance);

	vec3 finalColor;
	if(planetID >= 0)
	{
 		vec3 pt = cameraCoordAndClipZ.xyz - uPlanetCenter[planetID];
  		float height = length(pt) - uInnerRadius[planetID];
  		float sunAngle = acos(dot(normalize(pt), lightDir));
		vec2 OD;
  		vec3 Isky = get_Isky_and_optical_depth(sunAngle, height, planetID, OD);
   		vec3 expTval = exp(-uBeta[planetID] * OD.x) + exp(-uBeta[planetID] * OD.y);
		finalColor = lightIntesity * (diffuseCoeff * uLightColor * expTval * shadowCoeff + Isky * uLightColor);
	}
	else if(planetID == NO_PLANET)
	{
		finalColor = lightIntesity * diffuseCoeff * uLightColor * shadowCoeff;
	}
	else
	{
		finalColor = vec3(1);
	}
	//out_color = vec4(0, 0, 0, 1);
 	out_color = vec4(finalColor * albedo, 1);
	//out_color = vec4(shadow_coeff_test(cameraCoordAndClipZ.w, cameraCoordAndClipZ.xyz), 1);
	//out_color = vec4(1, 0, 1, 1);
}
