
#if !defined(__EXPLORE2_ATMOSPHERIC_H__)
#define __EXPLORE2_ATMOSPHERIC_H__

#include "Math/vector2.h"
#include "Math/vector3.h"
#include "Math/matrix3.h"
#include "Math/transformation.h"

namespace explore2 {;

struct Atmosphere 
{
	typedef float float_type;
	typedef math::Vector3<float_type> spectrum_type;
	typedef math::Vector3<float_type> vec3_type;
	typedef math::Vector2<float_type> vec2_type;
	typedef math::Matrix3<float_type> mat3_type;

	void set_radii(float_type inner, float_type outer)
	{
		_innerRadius = inner;
		_outerRadius = outer;
	}

	// scale height is the thickness of the atmosphere its density
	// were uniform (7994m for Earth's atmosphere)
	void set_scale_height(float_type scaleHeight)
	{
		_scaleHeight = scaleHeight;
	}

	// n is the index of refraction of the air
	// Ns is the molecular number density of the standard atmosphere
	void set_k(float_type n, float_type Ns)
	{
		_K = (float_type(2) * float_type(M_PI) * float_type(M_PI) * (n * n - float_type(1)) * (n * n - float_type(1))) / (float_type(3) * Ns);
	}

	// wavelength of light entering the atmosphere
	void set_light_wavelength(spectrum_type waveLength)
	{	
		_waveLength = waveLength;
		_beta = (float_type(4) * float_type(M_PI) * _K) / 
			float_type(::pow(waveLength, 4));
	}

private:
	// calculates the scattering phase function Fr for a particular angle
	static inline float_type get_scattering_phase_Fr(float_type angle)
	{
		float_type cosAngle = float_type(::cos(angle));
		return float_type(0.75) * (float_type(1) + cosAngle * cosAngle);
	}

	// calculates the density ratio rho (p) for a given height (h) and scale height (H0)
	static inline float_type get_density_ratio_rho(float_type h, float_type scaleHeight)
	{
		return float_type(::exp(-h / scaleHeight));
	}

	// over height instead of position
	// beta - result of the beta function
	// starth - starting height
	// endh - ending height
	// distance - distance from start point (where height is starth) to end point
	// innerRadius - the radius of the planet at height = 0
	// scaleHeight - the set scaleHeight
	// samples - number of samples to use during integration (trapezium) 
	static spectrum_type calculate_optical_depth_t(spectrum_type beta, float_type starth, float_type endh, float_type distance, float_type innerRadius, float_type scaleHeight, int samples)
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

		return beta * sampleSum;
	}

	static inline vec2_type calculate_end_pos(float_type starth, float_type endh, float_type radius, float_type distance)
	{
		assert(distance < (radius * 2 + starth + endh));
		float_type a = endh + radius, b = distance, c = starth + radius;
		float_type angle = float_type(::acos((c*c + a*a - b*b) / (2 * c * a)));
		float_type Cx = float_type(::cos(angle)) * a;
		float_type Cy = float_type(::sin(angle)) * a;
		return vec2_type(Cx, Cy);
	}

	static float_type interpolate_height(float_type starth, float_type endh, float_type radius, float_type distance, float_type fraction)
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
// 		assert(distance < (radius * 2 + starth + endh));
// 		float_type a = endh + radius, b = distance, c = starth + radius;
// 		float_type angle = float_type(::acos((c*c + a*a - b*b) / (2 * c * a)));
// 		float_type Cx = float_type(::cos(angle)) * a;
// 		float_type Cy = float_type(::sin(angle)) * a;
// 		vec2_type startPos(c, 0);
// 		vec2_type endPos(Cx, Cy);
// 		vec2_type pt = (endPos - startPos) * fraction + startPos;
// 		return pt.length() - radius;
		vec2_type startPos(starth + radius, 0);
		vec2_type endPos = calculate_end_pos(starth, endh, radius, distance);
		vec2_type pt = (endPos - startPos) * fraction + startPos;
		return pt.length() - radius;
	}

	// Is - irradiance entering atmosphere
	// theta - angle between viewer and sun vectors
	// heightPa - height at start point (Pa)
	// heightPb - height at end point (Pb)
	// distancePaPb - distance between Pa and Pb
	static spectrum_type calculate_irradiance_at_view_Iv(spectrum_type beta, spectrum_type waveLength, float_type Is, float_type theta, float_type heightPa, float_type heightPb, float_type distancePaPb, float_type innerRadius, float_type outerRadius, float_type scaleHeight, float_type K, int PaPbSteps, int PPaSteps, int PPcSteps)
	{
		float_type KFrThetaOverLambda4 = K * get_scattering_phase_Fr(theta) / float_type(::pow(waveLength, 4));
		// integrate from Pa to Pb
		float_type sampleWidth = distance / float_type(PaPbSteps + 1);

		vec2_type Pa = vec2_type(innerRadius + heightPa);
		vec2_type Pb = calculate_end_pos(heightPa, heightPb, innerRadius, distancePaPb);


// 		float_type sampleSum = get_density_ratio_rho(starth, scaleHeight) + get_density_ratio_rho(endh, scaleHeight);
// 		float_type sampleFrac = float_type(1) / float_type(samples + 1);
// 		float_type frac = sampleFrac;
// 		for(int sampleIdx = 0; sampleIdx < samples; ++sampleIdx, frac += sampleFrac)
// 		{
// 			float_type currh = interpolate_height(starth, endh, innerRadius, distance, frac);
// 			sampleSum += get_density_ratio_rho(currh, scaleHeight) * 2;
// 		}
// 		sampleSum *= (float_type(0.5) * sampleWidth);
	}

	// solve the out scattering from Pc to Pa
	static spectrum_type calculate_inner_integral(spectrum_type beta, vec2_type Pa, /*vec2_type Pb,*/ vec2_type P, float_type theta, float_type innerRadius, float_type outerRadius, float_type scaleHeight, int PPaSteps, int PPcSteps)
	{
		float_type heightP = P.length() - innerRadius;
		float_type heightPa = Pa.length() - innerRadius;

		vec2_type Pc = calculate_Pc(P, Pa, theta, outerRadius);
		float_type heightPc = Pc.length() - innerRadius;

		spectrum_type tPPc = calculate_optical_depth_t(beta, heightP, heightPc, (Pc - P).length(), innerRadius, scaleHeight, 
PPcSteps);
		spectrum_type tPPa = calculate_optical_depth_t(beta, heightP, heightPa, (Pa - P).length(), innerRadius, scaleHeight, 
			PPaSteps);
		float_type rhoP = get_density_ratio_rho(heightP, scaleHeight);
		return rhoP * value_type(::exp(-tPPc - tPPa));
	}

	static vec2_type calculate_Pc(vec2_type P, vec2_type Pa, float_type theta, float_type outerRadius)
	{
		// get vector rotated from P->Pa by theta
		vec2_type dirP = (Pa - P).normal();
		mat3_type rotateToSunDir = math::rotate_2D_rad(-theta);
		vec2_type sunDir(rotateToSunDir * vec3_type(dirP, float_type(0)));
		return first_circle_ray_isect(P, sunDir, outerRadius);
	}

	// assumes circle is centered on origin
	// solution from http://stackoverflow.com/questions/1073336/circle-line-collision-detection
	static inline vec2_type first_circle_ray_isect(const vec2_type& rayStart, const vec2_type& rayDir, float_type radius)
	{
		float_type a = rayDir.dotp(rayDir);
		float_type b = float_type(2) * rayStart.dotp(rayDir);
		float_type c = rayStart.dotp(rayStart) - radius * radius;

		float_type discriminant = b * b - float_type(4) * a * c;
		assert(discriminant >= 0);

		discriminant = ::sqrt( discriminant );
		//if( discriminant < 0 )
		//{
		//	// no intersection
		//}		
		float_type t1 = (-b + discriminant) / (float_type(2) * a);
		return rayStart + rayDir * t1;
		//float_type t2 = (-b - discriminant)/(2*a);
		//
		//if( t1 >= 0 && t1 <= 1 )
		//{
		//	// solution on is ON THE RAY.
		//}
		//else
		//{
		//	// solution "out of range" of ray
		//}

		//// use t2 for second point
	}

private:

	float_type _innerRadius, _outerRadius,
		_K, _scaleHeight;
	spectrum_type _waveLength, _beta;
};

}

#endif // __EXPLORE2_ATMOSPHERIC_H__