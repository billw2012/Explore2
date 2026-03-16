#ifndef atmospheric_scattering_h__
#define atmospheric_scattering_h__

#include <vector>

#include "Math/vector3.hpp"

namespace explore2 {;

struct AtmosphericScattering
{
	typedef float float_type;
	typedef math::Vector3<float_type> vec3_type;

	struct Parameters 
	{
		Parameters(float_type innerRadius_ = 0, float_type depth_ = 0, 
			float_type scaleHeightRayleigh_ = 6750, float_type scaleHeightMie_ = 1200, 
			float_type mieU_ = 0.8, float_type n_ = 2.5e25, float_type Ns_ = 1.0003)
			: innerRadius(innerRadius_),
			depth(depth_),
			scaleHeightRayleigh(scaleHeightRayleigh_),
			scaleHeightMie(scaleHeightMie_),
			mieU(mieU_),
			n(n_),
			Ns(Ns_)
		{

		}

		float_type innerRadius;
		float_type depth;
		float_type scaleHeightRayleigh;
		float_type scaleHeightMie;
		float_type mieU;
		float_type n;
		float_type Ns;
	};

	static void static_init();
	static vec3_type calculate_beta(float_type n, float_type Ns);
	static void calculate_tables(int samples, int angleSamples, int heightSamples, int KFrSamples, Parameters params,
		std::vector<float_type>& opticalDepthRayleighTable, std::vector<float_type>& opticalDepthMieTable,
		std::vector<vec3_type>& KFrRayleighTable, std::vector<vec3_type>& KFrMieTable, std::vector<vec3_type>& surfaceLightingTable);
};

};

#endif // atmospheric_scattering_h__

