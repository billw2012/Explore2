

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_albedo;

uniform sampler2D fontTexture;

void main()
{
	vec4 colour = texture(fontTexture, In.uv).rgba;
	out_albedo = vec4(colour);
}
