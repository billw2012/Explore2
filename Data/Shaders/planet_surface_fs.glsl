


#pragma include "misc.glsl"

in FragmentIn
{
	vec2 uv;
	vec4 posAndClipz;
} In;

//varying vec2 vUV;
//varying vec4 vCameraCoordXYZandClipZ;

//out FragmentStruct
//{
layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;
//} Out;

uniform sampler2D	uColourTexture;
uniform sampler2D	uNormalTexture;
uniform sampler2D	uHeightTexture;

uniform mat4		uModelView;

uniform int			uPlanetIndex;

void main()
{
	out_albedo = vec4(texture(uColourTexture, In.uv).rgb, uPlanetIndex);//uMaterialID);
	vec4 normal = vec4(decompress_normal(texture(uNormalTexture, In.uv).xyz), 0);
	vec4 cameraNormal = uModelView * normal;	
	out_normal = vec4(cameraNormal.xyz, texture(uHeightTexture, In.uv).x);//vec4(compress_normal(globalNormal.xyz), 1);	
	out_specular = vec4(0, 0, 1, 1);	
	out_posAndClipz = In.posAndClipz;
	gl_FragDepth = calculate_depth(In.posAndClipz.w);
}
