


#pragma include "misc.glsl"

in FragmentIn
{
	vec2 uv;
	vec4 posAndClipz;
} In;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;

uniform vec3 uColor;

void main()
{
	out_albedo = vec4(uColor * 100, NO_LIGHTING);
	//vec4 normal = vec4(decompress_normal(texture(uNormalTexture, In.uv).xyz), 0);
	//vec4 cameraNormal = uModelView * normal;	
	//out_normal = vec4(cameraNormal.xyz, texture(uHeightTexture, In.uv).x);//vec4(compress_normal(globalNormal.xyz), 1);	
	//out_specular = vec4(0, 0, 1, 1);	
	out_normal = vec4(0, 0, 0, 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.posAndClipz;
	gl_FragDepth = calculate_depth(In.posAndClipz.w);
}
