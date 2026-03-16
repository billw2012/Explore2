

//varying vec2	vUV;	

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_albedo;

#pragma include "gbuffer_textures.glsl"

void main()
{
	vec3 normal = texture(uNormalBufferTexture, In.uv).rgb;
	vec3 albedo = texture(uAlbedoBufferTexture, In.uv).rgb;
	//vec3 specular = texture(uSpecularBufferTexture, vUV).rgb;
	//float depth = texture(uDepthBufferTexture, vUV).r;
	out_albedo = vec4(albedo * normal /*+ specular*/, 1);
	//gl_FragColor = vec4(normal, 1);
}
