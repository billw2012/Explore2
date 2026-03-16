


uniform sampler2D uColourTexture;

layout(location = 0) out vec4 out_color;

in FragmentIn
{
	vec3 uv;
} In;


void main()
{
	vec4 texcol = texture(uColourTexture, In.uv.xy);
	//out_color = vec4(mod(In.uv.xy, 1), 0, 1);
// 	float edgecol = 0;
// 	if(mod(In.uv.x, 1) < 0.1)
// 		edgecol = 1;
	out_color = vec4(texcol.xyz /*+ vec3(edgecol)*/, In.uv.z);
}
