// HPCNoiseTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HPCNoise/perlin.hpp"
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/taus88.hpp>
#include <windows.h>
#include "Misc/timer.h"
#include "Math/perlin.hpp"
#include <ppl.h>
#if _MSC_VER >= 1700
#include <concurrent_vector.h>
#endif

#define NUM_VALS_BASE (32*32)
#define ITER 500
#define NUM_VALS (ITER * NUM_VALS_BASE)

int _tmain(int argc, _TCHAR* argv[])
{
	typedef float real_type;

	opencl::init();

	typedef HPCNoise::Noise<real_type> NoiseType;
	NoiseType::pts_vector_type ptsVec;
	{
		boost::random::uniform_real_distribution<real_type> dist(-1.0, 1.0);
		boost::taus88 rand;
		for(size_t idx = 0; idx < NUM_VALS_BASE; ++idx)
		{
			ptsVec.push_back(NoiseType::vector4_type(dist(rand), dist(rand), dist(rand), (real_type)0.0));
		}
	}

	double tCPUSingle;
	math::Noise<real_type> noiseProvider;
	{
		std::cout << "Performing work on CPU single core..." << std::endl;
		std::vector<real_type> results(ptsVec.size());
		misc::Timer timer;
		for(size_t idx = 0; idx < ITER; ++idx)
		{
			size_t ptsSize = ptsVec.size();
			for(size_t idx2 = 0; idx2 < ptsSize; ++idx2) 
			{
				const NoiseType::vector4_type& vec = ptsVec[idx2];
				results[idx2] = math::perlin<real_type>(noiseProvider, vec.x, vec.y, vec.z, 1.5, 16);
			}
		}
		tCPUSingle = timer.elapsed();
		std::cout << "Work complete in " << tCPUSingle << "ms, " << NUM_VALS / tCPUSingle << " vals/ms" << std::endl << std::endl;
	}
#if _MSC_VER >= 1700
	double tCPUMulti;
	{
		std::cout << "Performing work on CPU multi core..." << std::endl;
		concurrency::concurrent_vector<real_type> results(ptsVec.size());
		misc::Timer timer;
		for(size_t idx = 0; idx < ITER; ++idx)
		{
			size_t ptsSize = ptsVec.size();
			concurrency::parallel_for<size_t>(0, ptsSize, 1, [&](size_t idx2) {
				const NoiseType::vector4_type& vec = ptsVec[idx2];
				results[idx2] = math::perlin<real_type>(noiseProvider, vec.x, vec.y, vec.z, 1.5, 16);
			});
		}
		tCPUMulti = timer.elapsed();
		std::cout << "Work complete in " << tCPUMulti << "ms, " << NUM_VALS / tCPUMulti << " vals/ms" << std::endl << std::endl;
	}
#endif
	double tOpenCL;
	{
		NoiseType noise;

		noise.add_octave(NoiseType::OctavePtr(new NoiseType::BasicOctave(0.5, 3.0)), 16);
		noise.init(0);

		std::cout << "Performing work on OpenCL..." << std::endl;
		misc::Timer timer;
		NoiseType::NoiseExecutionPtr exec;
		for(size_t idx = 0; idx < ITER; ++idx)
			exec = noise.enqueue_work(ptsVec);
		const NoiseType::results_vector_type& results = exec->get_results();
		auto minMaxItrs = std::minmax_element(results.begin(), results.end());
		real_type baseMin = *(minMaxItrs.first);
		real_type baseMax = *(minMaxItrs.second);
		tOpenCL = timer.elapsed();
		std::cout << "Work complete in " << tOpenCL << "ms, " << NUM_VALS / tOpenCL << " vals/ms" << std::endl << std::endl;
	}

#if _MSC_VER >= 1700
	std::cout << "Speed ratio (single CPU / multi CPU): " << tCPUSingle / tCPUMulti << std::endl;
#endif
	std::cout << "Speed ratio (single CPU / OpenCL): " << tCPUSingle / tOpenCL << std::endl;
#if _MSC_VER >= 1700
	std::cout << "Speed ratio (multi CPU / OpenCL): " << tCPUMulti / tOpenCL << std::endl;
#endif

	return 0;
}

