

#pragma include "atmosphere_utils.glsl"

layout(location = 0) out vec4 out_albedo;

#define MAX_ATMOSPHERES 16
uniform mat4	uCameraToPlanetMatrix[MAX_ATMOSPHERES];
uniform vec3	uCameraPlanetLocal[MAX_ATMOSPHERES];
uniform float	uOuterRadiusSquared[MAX_ATMOSPHERES];
uniform float	uInnerRadius[MAX_ATMOSPHERES];
uniform float	uOneOverAtmosphereDepth[MAX_ATMOSPHERES];
uniform vec3	uBeta[MAX_ATMOSPHERES];
uniform int		uOpticalDepthRayleighTextureIndex[MAX_ATMOSPHERES];
uniform int		uOpticalDepthMieTextureIndex[MAX_ATMOSPHERES];

uniform sampler2DArray	uOpticalDepthRayleighTexture;
uniform sampler2DArray	uOpticalDepthMieTexture;

uniform sampler2DRect	uPBuffer;

uniform sampler2DRect	uAlbedoBufferTexture;
uniform sampler2DRect	uPositionBufferTexture;

// transform to planet local space..
void main()
{
	int planetID = int(texture(uAlbedoBufferTexture, gl_FragCoord.xy).w);

	vec3 tval;

	if(planetID >= 0)
	{
		vec4 cameraCoordAndClipZ = texture(uPositionBufferTexture, gl_FragCoord.xy);
		vec3 planetCoord = (uCameraToPlanetMatrix[planetID] * vec4(cameraCoordAndClipZ.xyz, 1)).xyz;
		vec3 coordDir = planetCoord - uCameraPlanetLocal[planetID];
		float coordDirLen = length(coordDir);
		coordDir /= coordDirLen;

 		vec2 t0t1 = sphere_intersection(uCameraPlanetLocal[planetID], coordDir, uOuterRadiusSquared[planetID], vec2(0, coordDirLen));

		//if(t0t1.x > t0t1.y)
		//{
		//	float temp = t0t1.x;
		//	t0t1.x = t0t1.y;
		//	t0t1.y = temp;
		//}
		vec3 Pa = uCameraPlanetLocal[planetID] + (t0t1.x * coordDir);
		vec3 Pb = uCameraPlanetLocal[planetID] + (min(coordDirLen, t0t1.y) * coordDir);

		//vec3 Pa = uCameraPlanetLocal[planetID];
		//vec3 Pb = uCameraPlanetLocal[planetID];
 		// attenuate the light from the fragment
 		tval =  exp(1 * -calculate_t(Pa, Pb, uBeta[planetID], uInnerRadius[planetID], uOneOverAtmosphereDepth[planetID], 
			uOpticalDepthRayleighTexture, uOpticalDepthRayleighTextureIndex[planetID],
			uOpticalDepthMieTexture, uOpticalDepthMieTextureIndex[planetID]));
 	}
	else
	{
		tval = vec3(1);
	}
	
	out_albedo = vec4(texture(uPBuffer, gl_FragCoord.xy).xyz * tval, 1);
	//out_albedo = vec4(1, 0, 0, 1);
}
