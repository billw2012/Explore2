
#version 330
#extension GL_EXT_texture_array : require

#pragma include "misc.glsl"

#pragma include "atmosphere_utils.glsl"

#pragma include "gbuffer_textures.glsl"

varying vec2	vUV;

uniform vec3	uLightDir;
uniform vec3	uLightColor;
uniform vec3	uPlanetCenter;
uniform float	uInnerRadius;
uniform float	uOneOverAtmosphereDepth;
uniform vec3	u4PiKOverWavelength4;
	
uniform sampler2DArrayShadow uShadowDepthTextures;
uniform sampler2D uSurfaceLightingTexture;
uniform sampler2D uOpticalDepthTexture;

uniform vec4	uFarDist;
uniform mat4	uLightFrustum0;
uniform mat4	uLightFrustum1;
uniform mat4	uLightFrustum2;
uniform mat4	uLightFrustum3;

float shadow_coeff(in float depth, in vec3 vPos)
{
	if(depth > uFarDist.w)
	{
		return 1.0;
	}
	
	int index = 0;
	mat4 lightFrustum = uLightFrustum0;
	float fade = 0;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(depth > uFarDist.z)
	{
		index = 3;
		lightFrustum = uLightFrustum3;
		fade = (depth - uFarDist.z) / (uFarDist.w - uFarDist.z);
	}
	else if(depth > uFarDist.y)
	{
		index = 2;
		lightFrustum = uLightFrustum2;
	}
	else if(depth > uFarDist.x)
	{
		index = 1;
		lightFrustum = uLightFrustum1;
	}
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadowCoord = lightFrustum * vec4(vPos, 1);
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

vec4 get_Isky_and_optical_depth(float sunAngle, float height)
{
 	float xv = height * uOneOverAtmosphereDepth;
 	float yv = sunAngle * AngleRange;
 	vec3 Isky = texture(uSurfaceLightingTexture, vec2(xv, yv)).rgb;
  	float opticalDepth = texture(uOpticalDepthTexture, vec2(xv, yv)).r;
  	return vec4(Isky, opticalDepth);
}

void main()
{
	const float ambientFactor = 0.5;

	vec4 cameraCoordAndClipZ = texture(uPositionBufferTexture, vUV);
	float shadowCoeff = shadow_coeff(cameraCoordAndClipZ.w, cameraCoordAndClipZ.xyz);

	vec3 normal = texture(uNormalBufferTexture, vUV).xyz;
	float diffuseCoeff = max(0, dot(normal, -uLightDir));

	vec3 pt = cameraCoordAndClipZ.xyz - uPlanetCenter;
 	float height = length(pt) - uInnerRadius;
 	float sunAngle = acos(dot(normalize(pt), -uLightDir));
 	vec4 IskyAndOD = get_Isky_and_optical_depth(sunAngle, height);
  	vec3 expTval = exp(-u4PiKOverWavelength4 * IskyAndOD.w);
  	vec3 finalColor = diffuseCoeff * uLightColor * expTval * shadowCoeff + IskyAndOD.xyz;
 	vec3 albedo = texture(uAlbedoBufferTexture, vUV).rgb;
	gl_FragColor = vec4(finalColor * albedo * 0.05, 1);
}
