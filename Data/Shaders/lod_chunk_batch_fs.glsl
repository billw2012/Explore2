#extension GL_EXT_texture_array : require

#pragma include "misc.glsl"

in FragmentIn
{
	vec3 textureUV;
	vec3 normalUV;
	vec4 cameraPosAndClipZ;
} In;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;


uniform sampler2DArray	uColourTexture;
uniform sampler2DArray	uNormalTexture;
	
uniform mat4			uModelView;

uniform float			uMaterialID;

void main()
{
	vec3 finalColour = texture2DArray(uColourTexture, In.textureUV).rgb;
	vec3 finalNormal = texture2DArray(uNormalTexture, In.normalUV).rgb;
	vec4 cameraNormal = uModelView * vec4(decompress_normal(finalNormal), 0);	
	
	//gl_FragData[ALBEDO_BUFFER_INDEX] = vec4(0.5, 0, 0, 1);
	out_albedo = vec4(finalColour, uMaterialID);
	out_normal = vec4(cameraNormal.xyz, 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.cameraPosAndClipZ;	
	
	gl_FragDepth = calculate_depth(In.cameraPosAndClipZ.w);
}
