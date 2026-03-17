
#include <cassert>
#include <algorithm>

#include "atmospheric_scattering.h"

#include "Math/constants.hpp"
#include "Math/vector2.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix3.hpp"
#include "Math/matrix4.hpp"
#include "Math/transformation.hpp"

#define EPSILON							200.0
#define QUARTER_SPHERE_SAMPLES			50
#define ATMOSPHERE_TABLE_ANGLE_DEGREES	110.0

namespace explore2 {;

namespace {;


using namespace math;

typedef AtmosphericScattering::float_type float_type;
typedef AtmosphericScattering::vec3_type vec3_type;
typedef Vector2<float_type> vec2_type;
typedef Vector4<float_type> vec4_type;
typedef Matrix3<float_type> mat3_type;
typedef Matrix4<float_type> mat4_type;


#define ATMOSPHERIC_SCALING 1.0
static std::vector<vec3_type> _quarterSphereSamples;
// light wavelengths * 1000
static const vec3_type LightWavelengths(float_type(0.000000700 * ATMOSPHERIC_SCALING), float_type(0.000000550 * ATMOSPHERIC_SCALING), float_type(0.000000460 * ATMOSPHERIC_SCALING));

template < class YTy_ >
vec3_type pow(const vec3_type& _X, YTy_ _Y)
{
	return vec3_type(std::pow(_X.x, _Y), std::pow(_X.y, _Y), std::pow(_X.z, _Y));
}

vec3_type exp(const vec3_type& _X)
{
	return vec3_type(std::exp(_X.x), std::exp(_X.y), std::exp(_X.z));
}

struct RayCircleIntersectRec
{
	struct Result { enum type {
		None,
		T1,
		T1andT2
	};};

	RayCircleIntersectRec(Result::type result_ = Result::None, float_type t1_ = 0, const vec2_type& p1_ = vec2_type(), float_type t2_ = 0, const vec2_type& p2_ = vec2_type()) 
		: result(result_), t1(t1_), t2(t2_), p1(p1_), p2(p2_) {}


	Result::type result;
	float_type t1, t2;
	vec2_type p1, p2;
};

RayCircleIntersectRec circle_ray_isect(const vec2_type& rayStart, const vec2_type& rayDir, float_type radius)
{
	float_type a = rayDir.dotp(rayDir);
	float_type b = float_type(2) * rayStart.dotp(rayDir);
	float_type c = rayStart.dotp(rayStart) - radius * radius;

	float_type discriminant = b * b - float_type(4) * a * c;

	discriminant = ::sqrt( discriminant );

	if(discriminant < float_type(0))
		return RayCircleIntersectRec();

	float_type t1 = (-b + discriminant) / (float_type(2) * a);
	// not sure this is much use, requires extra branches in calling code
	//if(discriminant == 0)
	//	return RayCircleIntersectRec(RayCircleIntersectRec::Result::T1,
	//		t1, rayStart + rayDir * t1);
	float_type t2 = (-b - discriminant) / (float_type(2) * a);
	if(t1 < t2)
		return RayCircleIntersectRec(RayCircleIntersectRec::Result::T1andT2,
		t1, rayStart + rayDir * t1, t2, rayStart + rayDir * t2);
	return RayCircleIntersectRec(RayCircleIntersectRec::Result::T1andT2,
		t2, rayStart + rayDir * t2, t1, rayStart + rayDir * t1);
}

struct RaySphereIntersectRec
{
	struct Result { enum type {
		None,
		T1,
		T1andT2
	};};

	RaySphereIntersectRec(Result::type result_ = Result::None, float_type t1_ = 0, const vec3_type& p1_ = vec3_type(), float_type t2_ = 0, const vec3_type& p2_ = vec3_type()) 
		: result(result_), t1(t1_), t2(t2_), p1(p1_), p2(p2_) {}

	Result::type result;
	float_type t1, t2;
	vec3_type p1, p2;
};

RaySphereIntersectRec sphere_ray_isect(const vec3_type& rayStart, const vec3_type& rayDir, float_type radius)
{
	//Compute A, B and C coefficients
	float_type a = rayDir.dotp(rayDir);
	float_type b = float_type(2) * rayDir.dotp(rayStart);
	float_type c = rayStart.dotp(rayStart) - (radius * radius);

	//Find discriminant
	float_type discriminant = b * b - float_type(4) * a * c;

	// if discriminant is negative there are no real roots, so return 
	// false as ray misses sphere
	if (discriminant < float_type(0))
		return RaySphereIntersectRec();

	if(discriminant == 0)
	{
		float_type q;
		if (b < float_type(0))
			q = (-b - discriminant)/float_type(2);
		else
			q = (-b + discriminant)/float_type(2);

		// compute t0 and t1
		float_type t0 = q / a;
		return RaySphereIntersectRec(RaySphereIntersectRec::Result::T1, t0, rayStart + rayDir * t0, t0, rayStart + rayDir * t0);
	}
	// compute q as described above
	discriminant = ::sqrt(discriminant);

	float_type q;
	if (b < float_type(0))
		q = (-b - discriminant)/float_type(2);
	else
		q = (-b + discriminant)/float_type(2);

	if(q == 0)
	{
		return RaySphereIntersectRec(RaySphereIntersectRec::Result::T1, float_type(0), rayStart, float_type(0), rayStart);
	}
	// compute t0 and t1
	float_type t0 = q / a;
	float_type t1 = c / q;

	if(t0 == t1)
		return RaySphereIntersectRec(RaySphereIntersectRec::Result::T1, t0, rayStart + rayDir * t0, t0, rayStart + rayDir * t0);
	if(t0 < t1)
		return RaySphereIntersectRec(RaySphereIntersectRec::Result::T1andT2, t0, rayStart + rayDir * t0, t1, rayStart + rayDir * t1);
	return RaySphereIntersectRec(RaySphereIntersectRec::Result::T1andT2, t1, rayStart + rayDir * t1, t0, rayStart + rayDir * t0);
}

void create_quarter_sphere_samples()
{
	while(_quarterSphereSamples.size() < QUARTER_SPHERE_SAMPLES)
	{
		vec3_type p(float_type(rand()) / float_type(RAND_MAX) - float_type(0.5), 
			float_type(rand()) / float_type(RAND_MAX) - float_type(0.5),
			float_type(rand()) / float_type(RAND_MAX) - float_type(0.5));
		if(p.x >= 0 && p.y >= 0)
		{
			p.normalize();
			_quarterSphereSamples.push_back(p);
		}
	}
}

// calculates the scattering phase function Fr for a particular angle
float_type get_scattering_phase_Fr(float_type angle, float_type g)
{
	float_type cosAngle = float_type(std::cos(angle));
	return ((float_type(3) * (float_type(1) - g * g)) / (float_type(2) * (float_type(2) + g * g))) * 
		((float_type(1) + cosAngle * cosAngle) / std::pow(float_type(1) + g * g - float_type(2) * g * cosAngle, float_type(3.0/2.0)));
}

// calculates the density ratio rho (p) for a given height (h) and scale height (H0)
float_type get_density_ratio_rho(float_type h, float_type scaleHeight)
{
	return float_type(::exp(-h / scaleHeight));
}

float_type calculate_k(float_type n, float_type Ns)
{
	return (float_type(2) * float_type(math::constants::pi) * float_type(math::constants::pi) * (n * n - float_type(1)) * (n * n - float_type(1))) / (float_type(3) * Ns);
}

vec2_type calculate_end_pos(float_type starth, float_type endh, float_type radius, float_type distance)
{
	assert(distance < (radius * 2 + starth + endh));
	float_type a = endh + radius, b = distance, c = starth + radius;
	float_type angle = float_type( ::acos( std::max( float_type(-1), 
		std::min( float_type(1), (c*c + a*a - b*b) / (float_type(2) * c * a) ) ) ) );
	float_type Cx = float_type(::cos(angle)) * a;
	float_type Cy = float_type(::sin(angle)) * a;
	return vec2_type(Cx, Cy);
}

float_type interpolate_height(float_type starth, float_type endh, float_type radius, float_type distance, float_type fraction)
{
	/*
	 C
	  +
	  |\
	  |  \
	  |    \  b
	a |      \
	  |        \
	  |angle     \
	  +-----------+ A
	 B      c
	
	using cosine rule for non right angle triangles, get the angle between the points
	formed by starth, endh and distance.
	*/
	vec2_type startPos(starth + radius, float_type(0));
	vec2_type endPos = calculate_end_pos(starth, endh, radius, distance);
	vec2_type pt = (endPos - startPos) * fraction + startPos;
	return pt.length() - radius;
}

float_type calculate_optical_depth(float_type starth, float_type endh, float_type distance, 
	float_type innerRadius, float_type scaleHeight, int samples)
{
	float_type sampleWidth = distance / float_type(samples + 1);
	float_type sampleSum = get_density_ratio_rho(starth, scaleHeight) + get_density_ratio_rho(endh, scaleHeight);
	float_type sampleFrac = float_type(1) / float_type(samples + 1);
	float_type frac = sampleFrac;
	for(int sampleIdx = 0; sampleIdx < samples; ++sampleIdx, frac += sampleFrac)
	{
		float_type currh = interpolate_height(starth, endh, innerRadius, distance, frac);
		sampleSum += get_density_ratio_rho(currh, scaleHeight) * 2;
	}
	sampleSum *= (float_type(0.5) * sampleWidth);

	return sampleSum;
}

float_type calculate_optical_depth_height_angle(float_type height, float_type angleFromNormal, 
												float_type innerRadius, float_type outerRadius, float_type scaleHeight, int samples)
{
	vec2_type startP(height + innerRadius - float_type(EPSILON), float_type(0));
	mat3_type rotMat(rotate_2D_rad(angleFromNormal));
	vec2_type rayDir(rotMat * vec3_type(startP, float_type(0)));
	vec2_type atmosIsect(circle_ray_isect(startP, rayDir, outerRadius).p2);
	float_type endh(atmosIsect.length() - innerRadius);
	float_type distance((atmosIsect - startP).length());
	return calculate_optical_depth(height, endh, distance, innerRadius, scaleHeight, samples);
}

float_type get_g(float_type u)
{
	float_type x = float_type(5.0/9.0) * u + float_type(125.0/729.0) * u * u * u + ::sqrt(float_type(64.0/27.0) - float_type(325.0/243.0) * u * u + float_type(1250.0/2187.0) * u * u * u * u);
	float_type xCubeRoot = ::pow(x, float_type(1.0/3.0));
	return float_type(5.0/9.0) * u - (float_type(4.0/3.0) - float_type(25.0/81.0) * u * u) * (float_type(1) / xCubeRoot) + xCubeRoot;
}

float_type interpolate_optical_depth(float_type angle, float_type height, 
									 float angleFracIn, int angleSamples, 
									 float heightFracIn, int heightSamples,
									 const std::vector<float>& opticalDepthTable)
{
	float_type angleFrac = angle / angleFracIn;
	float_type heightFrac = height / heightFracIn;
	float_type floorAngleFrac = floor(angleFrac);
	float_type floorHeightFrac = floor(heightFrac);
	float_type dAngleFrac = angleFrac - floorAngleFrac;
	float_type dHeightFrac = heightFrac - floorHeightFrac;
	int b = std::max<int>(0, std::min<int>(angleSamples-1, int(floorAngleFrac)));
	int t = std::max<int>(0, std::min<int>(angleSamples-1, int(ceil(angleFrac))));
	int l = std::max<int>(0, std::min<int>(heightSamples-1, int(floorHeightFrac)));
	int r = std::max<int>(0, std::min<int>(heightSamples-1, int(ceil(heightFrac))));
	int blIdx = b * angleSamples + l;
	int brIdx = b * angleSamples + r;
	int tlIdx = t * angleSamples + l;
	int trIdx = t * angleSamples + r;
	float_type topPt = opticalDepthTable[tlIdx] + (opticalDepthTable[trIdx] - opticalDepthTable[tlIdx]) * dHeightFrac; 
	float_type bottomPt = opticalDepthTable[blIdx] + (opticalDepthTable[brIdx] - opticalDepthTable[blIdx]) * dHeightFrac; 
	return bottomPt + (topPt - bottomPt) * dAngleFrac;
}

// solve the out scattering from Pc to Pa
vec3_type calculate_inner_integral(
	const vec3_type& beta, vec3_type Pa, vec3_type Pb, const vec3_type& P, 
	const vec3_type& sunDir, float_type innerRadius, float_type scaleHeight,
	float angleFracIn, int angleSamples, 
	float heightFracIn, int heightSamples,
	const std::vector<float>& opticalDepthTable)
{
	float_type heightP = P.length() - innerRadius;
	float_type heightPa = Pa.length() - innerRadius;
	float_type heightPb = Pb.length() - innerRadius;

	if(heightPb > heightPa)
	{
		std::swap(Pa, Pb);
		std::swap(heightPa, heightPb);
	}
	//vec3_type Pc = calculate_Pc(P, sunDir, outerRadius);
	//float_type heightPc = Pc.length() - innerRadius;

	//float_type angleFromNormalPc = (Pc - P).normal().angle(P.normal());
	vec3_type pNormal = P.normal();
	float_type angleFromLight = (-sunDir).angle(pNormal);
	vec3_type tPPc = beta * interpolate_optical_depth(angleFromLight, heightP, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);

	vec3_type PbPaNormal = (Pa - Pb).normal();
	float_type angleP_PbPa = PbPaNormal.angle(pNormal);
	float_type anglePa_PbPa = PbPaNormal.angle(Pa.normal());
	// DOING: Is this right? heightPc should always be 
	// outer atmosphere height. Should be depth from Pa - 
	// depth from P I think...
	float_type tPPaReminder = interpolate_optical_depth(anglePa_PbPa, heightPa, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);
	float_type tPPaFull = interpolate_optical_depth(angleP_PbPa, heightP, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);
	vec3_type tPPa = beta * (tPPaFull - tPPaReminder);

	float_type rhoP = get_density_ratio_rho(heightP, scaleHeight);
	return exp(-(tPPc + tPPa)) * rhoP;
}

// Is - irradiance entering atmosphere
// waveLength - wavelength of light
// beta - result of the beta function
// theta - angle between viewer and sun vectors
// Pa - end of ray nearest viewer (could be view position or intersection with atmosphere)
// Pb - end of ray furthest from viewer (could be intersection with solid object or atmosphere)
vec3_type calculate_irradiance_at_view_Iv(const vec3_type& beta, const vec3_type& sunDir, 
										  vec3_type Pa, vec3_type Pb, 
										  float_type innerRadius, 
										  float_type scaleHeight, float_type K, float_type g, 
										  int samples,
										  float angleFracIn, int angleSamples, 
										  float heightFracIn, int heightSamples,
										  const std::vector<float>& opticalDepthTable)
{
	float_type theta = sunDir.angle((Pa - Pb).normal());
	vec3_type KFrThetaOverLambda4 = K * get_scattering_phase_Fr(theta, g) / pow(LightWavelengths, 4);

	float_type distancePaPb = (Pa - Pb).length();

	// integrate from Pa to Pb
	float_type sampleWidth = distancePaPb / float_type(samples + 1);

	vec3_type sampleSum = 
		calculate_inner_integral(beta, Pa, Pb, Pa, sunDir, innerRadius, scaleHeight, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable) + 
		calculate_inner_integral(beta, Pa, Pb, Pb, sunDir, innerRadius, scaleHeight, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);
	float_type sampleFrac = float_type(1) / float_type(samples + 1);
	float_type frac = sampleFrac;
	for(int sampleIdx = 0; sampleIdx < samples; ++sampleIdx, frac += sampleFrac)
	{
		vec3_type P = Pa + (Pb - Pa) * frac;
		sampleSum += calculate_inner_integral(beta, Pa, Pb, P, sunDir, innerRadius, scaleHeight, angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable) * 2;
	}
	sampleSum *= (float_type(0.5) * sampleWidth);
	return KFrThetaOverLambda4 * sampleSum;
}

vec3_type irradiance_at_view_Iv(const vec3_type& sunDir, const vec3_type& eye, const vec3_type& eyeDir, 
	float_type g, float_type scaleHeight, float_type innerRadius, float_type outerRadius, float_type k, vec3_type beta, int samples,
	float angleFracIn, int angleSamples, float heightFracIn, int heightSamples, const std::vector<float>& opticalDepthTable)
{
	RaySphereIntersectRec atmosphereISect = sphere_ray_isect(eye, eyeDir, outerRadius);

	if(atmosphereISect.result == RaySphereIntersectRec::Result::None)
		return vec3_type::One;
	vec3_type startPos = atmosphereISect.t1 < 0 ? eye : atmosphereISect.p1;
	RaySphereIntersectRec planetISect = sphere_ray_isect(eye, eyeDir, innerRadius);
	// if there is no intersection with the planet or both intersection points are behind the eye
	if(planetISect.result == RaySphereIntersectRec::Result::None || 
		(planetISect.t1 <= 0 && planetISect.t2 <= 0))
	{
		// calculate from either the determined start position (either the eye or the first 
		// intersection with the atmosphere), to the second intersection point with the atmosphere
		return calculate_irradiance_at_view_Iv(beta, sunDir, startPos, atmosphereISect.p2, innerRadius, scaleHeight, k, g, samples,
			angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);
	}
	// check the eye isn't IN the planet
	assert(!(planetISect.t1 < 0 && planetISect.t2 > 0));
	// calculate from the determined start position to the first intersection with the planet
	return calculate_irradiance_at_view_Iv(beta, sunDir, startPos, planetISect.p1, innerRadius, scaleHeight, k, g, samples,
		angleFracIn, angleSamples, heightFracIn, heightSamples, opticalDepthTable);
}

void calculate_optical_depth_tables_internal(int samples, int angleSamples, int heightSamples, 
	const AtmosphericScattering::Parameters& params, 
	std::vector<float_type>& opticalDepthRayleighTable, std::vector<float_type>& opticalDepthMieTable)
{
	opticalDepthRayleighTable.resize(angleSamples * heightSamples);
	opticalDepthMieTable.resize(angleSamples * heightSamples);

	float_type fullHeight = params.depth;
	float_type heightFrac = fullHeight / float_type(heightSamples);
	float_type angleFrac = float_type(ATMOSPHERE_TABLE_ANGLE_DEGREES*math::constants::pi/180.0) / float_type(angleSamples);

	float_type angle = 0;
	for(int aIdx = 0, rIdx = 0; aIdx < angleSamples; ++aIdx, angle += angleFrac)
	{
		float_type height = 0;
		for(int hIdx = 0; hIdx < heightSamples; ++hIdx, height += heightFrac, ++rIdx)
		{
			opticalDepthRayleighTable[rIdx] = calculate_optical_depth_height_angle(height, angle, params.innerRadius, params.innerRadius + params.depth, params.scaleHeightRayleigh, samples);
			opticalDepthMieTable[rIdx] = calculate_optical_depth_height_angle(height, angle, params.innerRadius, params.innerRadius + params.depth, params.scaleHeightMie, samples);
		}
	}
}

std::vector<vec3_type> calculate_surface_lighting_table(int samples, 
	int angleSamples, int heightSamples, const AtmosphericScattering::Parameters& params,
	const std::vector<float>& opticalDepthTableRayleigh, const std::vector<float>& opticalDepthTableMie)
{
	std::vector<vec3_type> table(angleSamples * heightSamples);

	float_type heightFrac = params.depth / float_type(heightSamples);
	float_type angleFrac = float_type(ATMOSPHERE_TABLE_ANGLE_DEGREES*M_PI/180.0) / float_type(angleSamples);

	float_type k = calculate_k(params.n, params.Ns);
	vec3_type beta = AtmosphericScattering::calculate_beta(params.n, params.Ns);

	float_type angle(0);
	vec3_type sunDir(float_type(-1), float_type(0), float_type(0));
	float_type mieg = get_g(params.mieU);
	for(int aIdx = 0, rIdx = 0; aIdx < angleSamples; ++aIdx, angle += angleFrac)
	{
		float_type height = 0;

		vec3_type eyeDir(cos(angle), sin(angle), 0);
		mat4_type samplePtRotMat(rotatez_rad(angle));
		for(int hIdx = 0; hIdx < heightSamples; ++hIdx, height += heightFrac, ++rIdx)
		{
			vec3_type eyePos(eyeDir * (height + params.innerRadius + float_type(EPSILON)));

			vec3_type rayleigh;
			vec3_type mie;
			for(size_t sIdx = 0; sIdx < _quarterSphereSamples.size(); ++sIdx)
			{
				vec3_type samplePt(samplePtRotMat * vec4_type(_quarterSphereSamples[sIdx], (float_type)1));
				rayleigh += irradiance_at_view_Iv(sunDir, eyePos, samplePt, float_type(0), params.scaleHeightRayleigh, 
					params.innerRadius, params.innerRadius + params.depth, k, beta, samples, angleFrac, angleSamples, heightFrac, heightSamples, opticalDepthTableRayleigh);
				mie += irradiance_at_view_Iv(sunDir, eyePos, samplePt, mieg, params.scaleHeightMie, 
					params.innerRadius, params.innerRadius + params.depth, k, beta, samples, angleFrac, angleSamples, heightFrac, heightSamples, opticalDepthTableMie);
			}

			table[rIdx] = float_type(M_PI * 2) * (mie + rayleigh) / float_type(_quarterSphereSamples.size());
		}
	}

	return std::move(table);
}

float_type calculate_g(float_type u)
{
	float_type x = float_type(5.0/9.0) * u + float_type(125.0/729.0) * u * u * u + ::sqrt(float_type(64.0/27.0) - float_type(325.0/243.0) * u * u + float_type(1250.0/2187.0) * u * u * u * u);
	float_type xCubeRoot = std::pow(x, float_type(1.0/3.0));
	return float_type(5.0/9.0) * u - (float_type(4.0/3.0) - float_type(25.0/81.0) * u * u) * (float_type(1) / xCubeRoot) + xCubeRoot;
}

void calculate_KFr_tables(int samples, const AtmosphericScattering::Parameters& params, std::vector<vec3_type>& KFrRayleighTable, std::vector<vec3_type>& KFrMieTable)
{
	float_type k = calculate_k(params.n, params.Ns);
	float_type g = calculate_g(params.mieU);

	vec3_type oneOverpowWL4 = float_type(1) / pow(LightWavelengths, 4);

	KFrRayleighTable.resize(samples);
	KFrMieTable.resize(samples);

	float_type da = float_type(M_PI) / float_type(samples);
	float_type angle = 0;
	for(int sample = 0; sample < samples; ++sample, angle += da)
	{
		KFrRayleighTable[sample] = k * get_scattering_phase_Fr(angle, 0) * oneOverpowWL4; 
		KFrMieTable[sample] = k * get_scattering_phase_Fr(angle, g) * oneOverpowWL4; 
	}
}

};

void AtmosphericScattering::static_init()
{
	create_quarter_sphere_samples();
}


vec3_type AtmosphericScattering::calculate_beta(float_type n, float_type Ns)
{
	float_type k = calculate_k(n, Ns);
	return (float_type(4) * float_type(math::constants::pi) * k) / pow(LightWavelengths, 4);
}


void AtmosphericScattering::calculate_tables(int samples, int angleSamples, int heightSamples, int KFrSamples, Parameters params,
	std::vector<float_type>& opticalDepthRayleighTable, std::vector<float_type>& opticalDepthMieTable,
	std::vector<vec3_type>& KFrRayleighTable, std::vector<vec3_type>& KFrMieTable, std::vector<vec3_type>& surfaceLightingTable)
{
	params.scaleHeightMie		*= 1000;
	params.scaleHeightRayleigh	*= 1000;
	params.innerRadius			*= 1000;
	params.depth				*= 1000;
	calculate_optical_depth_tables_internal(samples, angleSamples, heightSamples, params, opticalDepthRayleighTable, opticalDepthMieTable);
	calculate_KFr_tables(KFrSamples, params, KFrRayleighTable, KFrMieTable);
	surfaceLightingTable = calculate_surface_lighting_table(samples, angleSamples, heightSamples, params, opticalDepthRayleighTable, opticalDepthMieTable);
}


}