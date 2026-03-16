


#pragma include "misc.glsl"

//varying vec4	vTextureUV;
//varying vec4	vNormalUV;
//varying vec4	vCameraCoordXYZandClipZ;
in FragmentIn
{
	vec2 textureUV;
	vec2 parentTextureUV;
	vec2 normalUV;
	vec2 parentNormalUV;
	vec4 cameraPosAndClipZ;
} In;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;

uniform float		uBlendFactor;
uniform sampler2D	uColourTexture;
uniform sampler2D	uNormalTexture;
uniform sampler2D	uParentColourTexture;
uniform sampler2D	uParentNormalTexture;

uniform mat4		uModelView;

uniform int			uPlanetIndex;

void main()
{
	vec3 colour = texture(uColourTexture, In.textureUV).rgb;
	vec3 normal = texture(uNormalTexture, In.normalUV).rgb;
	vec3 parentColour = texture(uParentColourTexture, In.parentTextureUV).rgb;
	vec3 parentNormal = texture(uParentNormalTexture, In.parentNormalUV).rgb;
	vec3 finalColour = mix(parentColour, colour, uBlendFactor);
	vec3 finalNormal = mix(parentNormal, normal, uBlendFactor);
	vec4 cameraNormal = uModelView * vec4(decompress_normal(finalNormal), 0);	
	
	out_albedo = vec4(finalColour, uPlanetIndex);
	out_normal = vec4(cameraNormal.xyz, 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.cameraPosAndClipZ;	
	
	gl_FragDepth = calculate_depth(In.cameraPosAndClipZ.w);
}
