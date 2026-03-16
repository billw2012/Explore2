


uniform mat4 uModelView;
uniform mat4 uProj;
uniform vec4 uViewport;

uniform float uSize;
uniform float uMinimumSize;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec2 in_offset;

out FragmentIn
{
	vec2 uv;
} Out;

void main()
{
	Out.uv = in_uv;
	vec4 eyeCoord = uModelView * vec4(in_vertex, 1);
	vec3 eyeDir = normalize(eyeCoord).xyz;
	vec3 leftDir = normalize(cross(eyeDir, vec3(0, 1, 0)));
	vec3 upDir = normalize(cross(leftDir, eyeDir));
	vec3 offsetScaled = (leftDir /** in_offset.x*/ + upDir /** in_offset.y*/) * uSize;
	vec4 clipOffset = uProj * (eyeCoord + vec4(offsetScaled, 0));
	vec4 clipCoord = uProj * eyeCoord;

	vec2 minPixelSize = abs(clipCoord.ww / uViewport.zw) * uMinimumSize;
	vec2 actualPixelSize = abs(clipOffset.xy - clipCoord.xy);
	vec2 finalPixelSize = max(minPixelSize, actualPixelSize);
	vec2 size = in_offset * finalPixelSize;
	gl_Position = clipCoord + vec4(size, 0, 0);
}
