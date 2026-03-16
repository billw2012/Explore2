
#pragma once

const float PI = 3.1415926535;
const float AngleRange = (1 / PI) * (180.0 / 110.0);//0.5 * (130.0 / 180.0);//(1 / PI) * (130.0 / 180.0);

/*
Atmospheric scattering:
ray = camera pos to fragment pos
find intersection points on atmosphere 
*/

float calculate_intensity(float distance)
{
	return 1 / (distance * distance);
}

vec2 sphere_intersection(
	in vec3 origin, 
	in vec3 rayDir, 
	in float radiusSquared,
	in vec2 defaultT1T2)
{
	vec3 l = -origin;
	float s = dot(l, rayDir);
	float l2 = dot(l, l);
	
	// ray start is outside sphere and ray faces away from sphere
	if(s < 0 && l2 > radiusSquared) 
	{
		return defaultT1T2;
	}
	float m2 = l2 - (s * s);
	// closest point on ray to sphere is outside sphere
	if(m2 > radiusSquared)
	{
		return defaultT1T2;
	}
	float q = sqrt(radiusSquared - m2);
	if(l2 < radiusSquared)
	{
		return vec2(defaultT1T2.x, s + q);
	}
	return vec2(s - q, s + q);
}

float get_optical_depth(in float sunAngle, in float height, in float oneOverAtmosphereDepth, in sampler2DArray opticalDepthTexture, int opticalDepthTextureIndex)
{
 	float xv = height * oneOverAtmosphereDepth;
 	float yv = sunAngle * AngleRange;
  	return texture(opticalDepthTexture, vec3(xv, yv, opticalDepthTextureIndex)).r;
}

vec3 calculate_t(in vec3 PaIn, in vec3 PbIn, in vec3 beta, 
	in float innerRadius, in float oneOverAtmosphereDepth, 
	in sampler2DArray opticalDepthRayleighTexture, int opticalDepthRayleighTextureIndex,
	in sampler2DArray opticalDepthMieTexture, int opticalDepthMieTextureIndex)
{
	float lenPa = length(PaIn);
	float lenPb = length(PbIn);
	vec3 Pa, Pb;
	if(lenPa > lenPb)
	{	
		Pa = PaIn;
		Pb = PbIn;
	}
	else
	{
		Pa = PbIn;
		Pb = PaIn;
		float tmp = lenPa;
		lenPa = lenPb;
		lenPb = tmp;
	}
	
	float heightPa = max(0, lenPa - innerRadius);
	float heightPb = max(0, lenPb - innerRadius);
	
	vec3 normalPa = normalize(Pa);
	vec3 normalPb = normalize(Pb);
	vec3 normalPbPa = normalize(Pa-Pb);
	
	float anglePaToPaPb = acos(dot(normalPa, normalPbPa));
	float anglePbToPaPb = acos(dot(normalPb, normalPbPa));
	
	float tPbPaReminderRayleigh = get_optical_depth(anglePaToPaPb, heightPa, oneOverAtmosphereDepth, opticalDepthRayleighTexture, opticalDepthRayleighTextureIndex); 
	float tPbPaReminderMie = get_optical_depth(anglePaToPaPb, heightPa, oneOverAtmosphereDepth, opticalDepthMieTexture, opticalDepthMieTextureIndex); 
	float tPbPaFullRayleigh = get_optical_depth(anglePbToPaPb, heightPb, oneOverAtmosphereDepth, opticalDepthRayleighTexture, opticalDepthRayleighTextureIndex);
	float tPbPaFullMie = get_optical_depth(anglePbToPaPb, heightPb, oneOverAtmosphereDepth, opticalDepthMieTexture, opticalDepthMieTextureIndex);
	
	return beta * (tPbPaFullRayleigh - tPbPaReminderRayleigh) + beta * (tPbPaFullMie - tPbPaReminderMie);
}

float get_density_ratio_rho(float h, float scaleHeight)
{
	return exp(-h / scaleHeight);
}

