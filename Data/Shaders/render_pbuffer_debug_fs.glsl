

//varying vec2	vUV;	

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_albedo;

uniform float			uExposureCoeff;
uniform sampler2DRect	uPBuffer;

uniform int				uDebugRenderMode;

uniform sampler2DRect	uAlbedoBufferTexture;
uniform sampler2DRect	uNormalBufferTexture;
uniform sampler2DRect	uPositionBufferTexture;
uniform sampler2DRect	uDepthBufferTexture;
uniform sampler2DRect	uPBuffer0;

#define DEBUG_RENDER_OFF			0
#define DEBUG_RENDER_ALBEDO			1
#define DEBUG_RENDER_PLANET_INDEX	2
#define DEBUG_RENDER_NORMAL			3
#define DEBUG_RENDER_POSITION		4
#define DEBUG_RENDER_CLIPZ			5
#define DEBUG_RENDER_DEPTH			6
#define DEBUG_RENDER_HEIGHT			7
#define DEBUG_RENDER_ALL			8
#define DEBUG_RENDER_PBUFFER0		9
#define DEBUG_RENDER_PBUFFER1		10

#define PLANET_COLORS		14

const vec3 PlanetIndexColors[] = 
vec3[](
	vec3(0, 0, 0),
	vec3(0.5, 0.5, 0.5),
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1),
	vec3(0.5, 0, 0),
	vec3(0, 0.5, 0),
	vec3(0, 0, 0.5),
	vec3(0.25, 0, 0),
	vec3(0, 0.25, 0),
	vec3(0, 0, 0.25),
	vec3(0.125, 0, 0),
	vec3(0, 0.125, 0),
	vec3(0, 0, 0.125)
);

vec3 get_albedo_color(vec2 uv)
{
	return texture(uAlbedoBufferTexture, uv).rgb;
}

vec3 get_planet_index(vec2 uv)
{
	int planetIndex = int(texture(uAlbedoBufferTexture, uv).a);
	return PlanetIndexColors[int(mod(planetIndex + 2, PLANET_COLORS))];
	//if(planetIndex == -1)
	//	return vec3(
	//return vec3(1 - (( + 1) / (MAX_ATMOSPHERES + 1)));
}

vec3 get_normal(vec2 uv)
{
	return texture(uNormalBufferTexture, uv).rgb * 0.5 + 0.5;
}

vec3 get_position(vec2 uv)
{
	return mod(texture(uPositionBufferTexture, uv).rgb / 10000, 1);
}

vec3 get_clipz(vec2 uv)
{
	return vec3(texture(uPositionBufferTexture, uv).a / 1);
}

vec3 get_height(vec2 uv)
{
	return vec3(texture(uNormalBufferTexture, uv).a);
}

vec3 get_depth(vec2 uv)
{
	return vec3(mod(texture(uDepthBufferTexture, uv).r, 1));
}

vec3 expose(vec3 v)
{
	return vec3(1) - exp(-uExposureCoeff * v);
}

void main()
{
	vec3 color;
	if(uDebugRenderMode == DEBUG_RENDER_OFF || uDebugRenderMode == DEBUG_RENDER_PBUFFER1)
	{
		color = expose(texture(uPBuffer, gl_FragCoord.xy).rgb);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_PBUFFER0)
	{
		color = expose(texture(uPBuffer0, gl_FragCoord.xy).rgb);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_ALBEDO)
	{
		color = get_albedo_color(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_PLANET_INDEX)
	{
		color = get_planet_index(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_NORMAL)
	{
		color = get_normal(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_POSITION)
	{
		color = get_position(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_CLIPZ)
	{
		color = get_clipz(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_DEPTH)
	{
		color = get_depth(gl_FragCoord.xy);
	}
	else if(uDebugRenderMode == DEBUG_RENDER_HEIGHT)
	{
		color = expose(get_height(gl_FragCoord.xy));
	}
	else if(uDebugRenderMode == DEBUG_RENDER_ALL)
	{
		vec2 screenSizeOver2 = textureSize(uAlbedoBufferTexture, 0) / 2;

		if(gl_FragCoord.x <= screenSizeOver2.x)
		{
			if(gl_FragCoord.y <= screenSizeOver2.y)
			{
				color = get_albedo_color(gl_FragCoord.xy * 2);
			}
			else
			{
				color = get_normal((gl_FragCoord.xy - vec2(0, screenSizeOver2.y)) * 2);
			}
		}
		else
		{
			if(gl_FragCoord.y <= screenSizeOver2.y)
			{
				color = get_position((gl_FragCoord.xy - vec2(screenSizeOver2.x, 0)) * 2);
			}
			else
			{
				color = get_depth((gl_FragCoord.xy - screenSizeOver2) * 2);
			}
		}
	}

	out_albedo = vec4(color, 1);
}
