#ifdef USE_DOUBLE_PRECISION
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#define REAL  double
#define REAL2 double2
#define REAL3 double3
#define convert_REAL3 convert_double3
#define REAL4 double4
#else
#define REAL  float
#define REAL2 float2
#define REAL3 float3
#define convert_REAL3 convert_float3
#define REAL4 float4
#endif

REAL2 fade2(REAL2 t)
{ 
	return (t * t * t * (t * (t * (REAL)6 - (REAL)15) + (REAL)10)); 
}

REAL3 fade3(REAL3 t)
{ 
	return (t * t * t * (t * (t * (REAL)6 - (REAL)15) + (REAL)10)); 
}

REAL fade(REAL t)
{ 
	return (t * t * t * (t * (t * (REAL)6 - (REAL)15) + (REAL)10)); 
}

REAL lerp(REAL t, REAL a, REAL b) 
{ 
	return a + t * (b - a); 
}

// 2d noise function
REAL noisep2(REAL2 xy, constant unsigned short* p, constant REAL* ms_grad4)
{
	REAL2 fXY = floor(xy);
	int2 XY = convert_int2(fXY) & 255;
	xy -= fXY;
	REAL2 uv = fade2(xy);
	int A = p[XY.x  ]+XY.y;   // HASH COORDINATES 
	int	B = p[XY.x+1]+XY.y;
	return lerp(uv.y, 
		lerp(uv.x, ms_grad4[A], ms_grad4[B]), 
		lerp(uv.x, ms_grad4[A+1], ms_grad4[B+1])); 
}


// 3d noise function
REAL noisep3(REAL3 xyz, constant unsigned short* p, constant REAL* ms_grad4)
{
	int3 fXYZ = convert_int3(floor(xyz));
	int3 XYZ = fXYZ & (int3)255;
	xyz -= convert_REAL3(fXYZ);
	REAL3 uvw = fade3(xyz);
	int A = p[XYZ.x  ]+XYZ.y, AA = p[A]+XYZ.z, AB = p[A+1]+XYZ.z;   // HASH COORDINATES 
	int	B = p[XYZ.x+1]+XYZ.y, BA = p[B]+XYZ.z, BB = p[B+1]+XYZ.z;
	return lerp(uvw.z, 
		lerp(uvw.y, lerp(uvw.x, ms_grad4[AA], ms_grad4[BA]), lerp(uvw.x, ms_grad4[AB], ms_grad4[BB])),
		lerp(uvw.y, lerp(uvw.x, ms_grad4[AA + 1], ms_grad4[BA + 1]), lerp(uvw.x, ms_grad4[AB + 1], ms_grad4[BB + 1]))); 
}


// A basic perlin noise function for utility
REAL perlin_local(const REAL3 vxyz, const REAL persistence, const REAL lacunarity, 
	const int octaves, constant unsigned short* p, constant REAL* ms_grad4)
{
	REAL total = 0;
	REAL frequency = 1;
	REAL amplitude = 1;
	for(unsigned int i = 0; i < octaves; ++i)
	{
		total = total + noisep3(vxyz * frequency, p, ms_grad4) * amplitude;
		// is this faster than using pow function? probably..
		frequency = frequency * lacunarity;
		amplitude = amplitude * persistence;
	}

	return total;
}

// A basic perlin noise function for utility
//REAL perlin_local_ms_grad4_eval(const REAL ms_grad_val, const REAL persistence, const REAL lacunarity, 
//	const int octaves)
//{
//	REAL total = 0;
//	REAL frequency = 1;
//	REAL amplitude = 1;
//	for(unsigned int i = 0; i < octaves; ++i)
//	{
//		total = total + ms_grad_val * amplitude;
//		// is this faster than using pow function? probably..
//		frequency = frequency * lacunarity;
//		amplitude = amplitude * persistence;
//	}
//
//	return total;
//}


#undef FRACTAL_NOISE
#undef NOISE
#define FRACTAL_NOISE(_p, _l, _o) perlin_local(vxyz, _p, _l, _o, p, ms_grad4)
#define NOISE() noisep3(vxyz*frequency, p, ms_grad4)

kernel void perlin(global const REAL3* inPositions, global REAL* outResults, 
	const REAL offset, const REAL scale, constant unsigned short* p, constant REAL* ms_grad4)
{
	int currIdx = get_global_id(0);
	if(currIdx >= get_global_size(0))
		return ;

	REAL3 vxyz = inPositions[currIdx];

	REAL total = 0;
	REAL frequency = 1;
	REAL amplitude = 1;

	INSERT_POINT_perlin

	outResults[currIdx] = (total + offset) * scale;
}

#undef FRACTAL_NOISE
#undef NOISE
#define FRACTAL_NOISE(_p, _l, _o) perlin_local_ms_grad4_eval(msgradval, _p, _l, _o)
#define NOISE() msgradval

kernel void perlin_ms_grad4_eval(global REAL* outResults, constant REAL* ms_grad4)
{
	int currIdx = get_global_id(0);
	if(currIdx >= get_global_size(0))
		return ;

	REAL msgradval = ms_grad4[currIdx];

	REAL total = 0;
	REAL frequency = 1;
	REAL amplitude = 1;
		
	INSERT_POINT_ms_grad4_eval

	outResults[currIdx] = total;
}


/*
kernel void perlin_ms_grad4_eval(global REAL* outResults, REAL persistence, REAL lacunarity, constant REAL* ms_grad4)
{
	int currIdx = get_global_id(0);
	if(currIdx >= get_global_size(0))
		return ;

	const unsigned int octaves = 16;

	REAL msgradval = ms_grad4[currIdx];
	REAL total = 0;
	REAL frequency = 1;
	REAL amplitude = 1;
	for(unsigned int i = 0; i < octaves; ++i)
	{
		total = total + msgradval * amplitude;
		// is this faster than using pow function? probably..
		frequency = frequency * lacunarity;
		amplitude = amplitude * persistence;
	}

	outResults[currIdx] = total;
}

kernel void perlin(global const REAL3* inPositions, global REAL* outResults, 
	REAL persistence, REAL lacunarity, REAL offset, REAL scale, constant unsigned short* p, constant REAL* ms_grad4)
{
	int currIdx = get_global_id(0);
	if(currIdx >= get_global_size(0))
		return ;

	REAL3 vxyz = inPositions[currIdx];

	const unsigned int octaves = 16;
	REAL total = 0;
	REAL frequency = 1;
	REAL amplitude = 1;
	for(unsigned int i = 0; i < octaves; ++i)
	{
		total = total + noisep3(vxyz * frequency, p, ms_grad4) * amplitude;
		// is this faster than using pow function? probably..
		frequency = frequency * lacunarity;
		amplitude = amplitude * persistence;
	}

	outResults[currIdx] = (total + offset) * scale;
}
*/