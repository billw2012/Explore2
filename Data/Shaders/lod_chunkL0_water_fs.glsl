


#pragma include "misc.glsl"

uniform sampler2D	uParentColourTexture;
uniform sampler2D	uParentNormalTexture;
uniform float		uBlendFactor;

in FragmentIn
{
	vec3 normal;
	vec4 uv;
	vec4 cameraPosAndClipZ;
	//float clipZ;
} In;

// varying vec4	vUV;
// varying vec3	vNormal;
// varying vec4	vCameraCoordXYZandClipZ;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;

uniform mat4	uModelView;

uniform int	uPlanetIndex;

void main()
{
	vec3 parentColour = texture(uParentColourTexture, In.uv.zw).rgb;
	vec3 waterColour = vec3(0, 0, 1);
	vec3 finalColour = mix(parentColour, waterColour, uBlendFactor);
	
	vec3 parentNormal = decompress_normal(texture(uParentNormalTexture, In.uv.zw).rgb);
	vec3 finalNormal = mix(parentNormal, In.normal, uBlendFactor);
	vec4 cameraLocalNormal = uModelView * vec4(finalNormal, 0);
	
	out_albedo = vec4(finalColour, uPlanetIndex);
	//out_albedo = vec4(uBlendFactor, uBlendFactor, uBlendFactor, uPlanetIndex);
	out_normal = vec4(normalize(cameraLocalNormal.xyz), 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.cameraPosAndClipZ;	
	gl_FragDepth = calculate_depth(In.cameraPosAndClipZ.w);
}
