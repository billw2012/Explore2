

//varying vec2	vUV;	

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_albedo;

uniform sampler2DRect	uPBuffer;
uniform float			uExposureCoeff;

void main()
{
	vec3 colour = texture(uPBuffer, In.uv).rgb;
	out_albedo = vec4(vec3(1) - exp(-uExposureCoeff * colour), 1);
}
