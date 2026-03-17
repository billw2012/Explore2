
uniform sampler2D uBackgroundTexture;

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_color;

uniform vec4 uColor;
uniform sampler2D uTexture;

void main()
{
	float val = texture2D(uTexture, In.uv).r;
	// float backgroundCol = texture2D(uColourTexture, In.uv);
	out_color = uColor * val;
}
