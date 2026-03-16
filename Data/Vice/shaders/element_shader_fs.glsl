
uniform sampler2D uBackgroundTexture;

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_color;

uniform vec4 uColor;

void main()
{
	// float backgroundCol = texture2D(uColourTexture, In.uv);
	out_color = uColor;
}
