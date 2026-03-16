

uniform sampler2DRect uAlbedoBufferTexture;
uniform sampler2DRect uPositionBufferTexture;
uniform sampler2DRect uNormalBufferTexture;

#pragma include "atmosphere_utils.glsl"

layout(location = 0) out vec4 out_albedo;

uniform mat4	uCameraToPlanetMatrix;
uniform vec3	uCameraPlanetLocal;
uniform float	uOuterRadiusSquared;
uniform vec3	uPlanetLocalLightPos;
uniform float	uInnerRadius;
uniform float	uOneOverAtmosphereDepth;
uniform vec3	uBeta;
uniform float	uScaleHeightRayleigh;
uniform float	uScaleHeightMie;
uniform vec3	uIs;
uniform int		uPlanetIndex;

uniform int		uOpticalDepthRayleighTextureIndex;
uniform int		uOpticalDepthMieTextureIndex;
uniform int		uKFrMieTextureIndex;
uniform int		uKFrRayleighTextureIndex;


uniform sampler2DArray uOpticalDepthRayleighTexture;
uniform sampler2DArray uOpticalDepthMieTexture;
uniform sampler1DArray uKFrRayleighTexture;
uniform sampler1DArray uKFrMieTexture;

/*
Atmospheric scattering:
ray = camera pos to fragment pos
find intersection points on atmosphere 
*/

const int SAMPLES = 5;
const float FSAMPLES = 5;
const float SAMPLE_FRACTION = 1 / (FSAMPLES + 1);

vec3 safe_clamp(in vec3 f)
{
 	if(isnan(f.r) || isnan(f.g) || isnan(f.b))
		return vec3(100, 0, 0);
 	if(isinf(f.r) || isinf(f.g) || isinf(f.b))
		return vec3(0, 0, 100);
// 		return 0;
// 	if(isinf(f))
// 		return 1000000;
	return f;
}

mat2x3 calculate_inner_integral(vec3 inPa, vec3 inPb, vec3 P, vec3 lightDir)
{
	float heightPa = max(0, length(inPa) - uInnerRadius);
	float heightPb = max(0, length(inPb) - uInnerRadius);
	
	vec3 Pa, Pb;
 	if(heightPa >= heightPb)
 	{
		Pa = inPa;
		Pb = inPb;
 	}
 	else
 	{
 		float tmp = heightPa;
 		heightPa = heightPb;
 		heightPb = tmp;
 		Pa = inPb;
 		Pb = inPa;
 	}
	
	float heightP = max(0, length(P) - uInnerRadius);
	
	vec3 normalP = normalize(P);
	vec3 normalPa = normalize(Pa);
	
	float angleFromLight = acos(dot(normalP, lightDir));

// 	if(angleFromLight > PI * 0.5)
// 		return vec3(0);
// 	else
// 		return vec3(1);

	float inscatterFacR = get_optical_depth(angleFromLight, heightP, uOneOverAtmosphereDepth, uOpticalDepthRayleighTexture, uOpticalDepthRayleighTextureIndex);
	float inscatterFacM = get_optical_depth(angleFromLight, heightP, uOneOverAtmosphereDepth, uOpticalDepthMieTexture, uOpticalDepthMieTextureIndex);

	vec3 tPPcR = uBeta * inscatterFacR;
	vec3 tPPcM = uBeta * inscatterFacM;
	
	vec3 PbPaNormal = normalize(Pa - Pb);
	
	float anglePPbPa = acos(dot(normalP, PbPaNormal));
	float anglePaPbPa = acos(dot(normalPa, PbPaNormal));
	
	float tPPaReminderR = get_optical_depth(anglePaPbPa, heightPa, uOneOverAtmosphereDepth, uOpticalDepthRayleighTexture, uOpticalDepthRayleighTextureIndex); 
	float tPPaReminderM = get_optical_depth(anglePaPbPa, heightPa, uOneOverAtmosphereDepth, uOpticalDepthMieTexture, uOpticalDepthMieTextureIndex); 
	
	float tPPaFullR = get_optical_depth(anglePPbPa, heightP, uOneOverAtmosphereDepth, uOpticalDepthRayleighTexture, uOpticalDepthRayleighTextureIndex);
	float tPPaFullM = get_optical_depth(anglePPbPa, heightP, uOneOverAtmosphereDepth, uOpticalDepthMieTexture, uOpticalDepthMieTextureIndex);
	
	vec3 tPPaR = uBeta * (tPPaFullR - tPPaReminderR);
	vec3 tPPaM = uBeta * (tPPaFullM - tPPaReminderM);
	
	float rhoPR = get_density_ratio_rho(heightP, uScaleHeightRayleigh);
	float rhoPM = get_density_ratio_rho(heightP, uScaleHeightMie);
	
	return mat2x3(rhoPR * exp(- (tPPaR + tPPcR)), rhoPM * exp(- (tPPaM + tPPcM)));
}

// transform to planet local space..
void main()
{
	int planetIndex = int(texture(uAlbedoBufferTexture, gl_FragCoord.xy).w);
	//if(planetIndex < 0)
	//{
	//	out_albedo = vec4(0);
	//	//return ;
	//}
	vec4 cameraCoordAndClipZ = texture(uPositionBufferTexture, gl_FragCoord.xy);
	float height = texture(uNormalBufferTexture, gl_FragCoord.xy).w;
	vec3 coordPlanetLocal = (uCameraToPlanetMatrix * vec4(cameraCoordAndClipZ.xyz, 1)).xyz;
	if(planetIndex == uPlanetIndex && height > 1)
	{
		//out_albedo = vec4(vec3(abs(length(coordPlanetLocal) - height)), 1);
		//return ;
		coordPlanetLocal = normalize(coordPlanetLocal) * height;
	}
	vec3 coordDir = coordPlanetLocal - uCameraPlanetLocal;
	float coordDirLen = length(coordDir);
	if(coordDirLen == 0)
	{
		discard;
		return;
	}
	coordDir /= coordDirLen;

	vec2 t0t1 = sphere_intersection(uCameraPlanetLocal, coordDir, uOuterRadiusSquared, vec2(0, 0));
	
	//out_albedo = vec4(vec3(t0t1.x), 1);
	//return ;

	vec3 Pa = uCameraPlanetLocal + t0t1.x * coordDir;

	// if the first intersect is further than the hit point then the atmosphere is occluded so discard
	if(length(Pa - uCameraPlanetLocal) > length(coordPlanetLocal - uCameraPlanetLocal))
	{
		//out_albedo = vec4(0);
		discard;
		return;
	}
	vec3 Pb = uCameraPlanetLocal + min(coordDirLen, t0t1.y) * coordDir;

	// use an intersect point for light dir, NOT the coord location, as it may be way away from the planet surface in the case of a background pixel
	vec3 lightVec = uPlanetLocalLightPos - Pa;
	float lightDistance = length(lightVec);
	vec3 lightDir = lightVec / lightDistance;
	
	vec3 PaPb = (Pb - Pa);
	float PaPbLength = length(PaPb);

	//out_albedo = vec4(vec3(PaPbLength), 1);
	//return ;

	mat2x3 sampleSum = calculate_inner_integral(Pa, Pb, Pa, lightDir) + calculate_inner_integral(Pa, Pb, Pb, lightDir); 
	
 	float sampleWidth = PaPbLength * SAMPLE_FRACTION; 
 	vec3 dv = PaPb * SAMPLE_FRACTION;
 	vec3 P = Pa + dv;
 	
 	for(int i = 0; i < SAMPLES; i++)
 	{
 		sampleSum += calculate_inner_integral(Pa, Pb, P, lightDir) * 2;		
 		P += dv;
 	}
	sampleSum *= (0.5 * sampleWidth);
	//vec3 alertCol = vec3(1);
	//if(isnan(sampleWidth))
	//	alertCol = vec3(10000, 0, 0);
	//if(isinf(sampleWidth))
	//	alertCol = vec3(0, 0, 1000);
	//sampleSum[0] = alertCol;
	//sampleSum[1] = alertCol;
	
	float theta = acos(dot(lightDir, normalize(PaPb)));
	float yv = theta / PI;
	
	vec3 KFrMie = texture(uKFrMieTexture, vec2(yv, uKFrMieTextureIndex)).rgb;
	vec3 KFrRayleigh = texture(uKFrRayleighTexture, vec2(yv, uKFrRayleighTextureIndex)).rgb;
	vec3 finalIs = uIs /** 20*/;
	float lightIntensity = calculate_intensity(lightDistance);
	vec3 colour = lightIntensity * finalIs * (KFrRayleigh * sampleSum[0] * 1000 + KFrMie * sampleSum[1] * 1000);
	
	vec3 col2 = safe_clamp(colour);//vec3(safe_clamp(colour.r), safe_clamp(colour.g), safe_clamp(colour.b));

	out_albedo = vec4(col2, 1);
	//out_albedo = vec4(0, 0, 0, 0);
}
