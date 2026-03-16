

uniform sampler2DArray uColourTexture;

//varying vec2 vUV;
in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_color;

uniform vec2 uScale;

void main()
{
	float texcol = texture2DArray(uColourTexture, vec3(In.uv * uScale, 0)).r;
	out_color = vec4(texcol, texcol, texcol, 1);
}
