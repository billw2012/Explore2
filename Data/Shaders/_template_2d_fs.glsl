in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_albedo;

uniform sampler2DRect	texture;

void main()
{
	vec4 colour = texture(texture, In.uv);
	out_albedo = vec4(colour);
}
