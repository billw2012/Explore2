


#pragma include "misc.glsl"

in FragmentIn
{
	vec2 uv;
} In;

uniform sampler2D uTexture;

layout(location = 0) out vec4 out_albedo;

void main()
{
	vec4 color = texture(uTexture, In.uv);
	out_albedo = color;//vec4(vec3(1), NO_LIGHTING);
}
