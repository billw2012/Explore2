#ifndef perlin_h__
#define perlin_h__

#include "HPCLib/OpenCLWrappers.h"
#include "Math/vector4.hpp"
#include <iostream>
#include <boost/thread/mutex.hpp>

namespace HPCNoise {;

void init();

namespace internal {;

const std::vector<std::string>& get_shader_base();

};

template <class FloatType>
struct NoiseFloatTraits {};

template <>
struct NoiseFloatTraits<float>
{
	typedef float real_type;
	typedef cl_float cl_real_type;
	typedef cl_float2 cl_real2_type;
	typedef cl_float3 cl_real3_type;
	typedef cl_float4 cl_real4_type;
};

template <>
struct NoiseFloatTraits<double>
{
	typedef double real_type;
	typedef cl_double cl_real_type;
	typedef cl_double2 cl_real2_type;
	typedef cl_double3 cl_real3_type;
	typedef cl_double4 cl_real4_type;
};

template < class InType_, class OutType_ >
struct Execution
{
	template < class FloatTy_ >
	friend struct Noise;
	typedef InType_ in_type;
	typedef OutType_ out_type;
	typedef Execution<InType_, OutType_> this_type;

	typedef opencl::OpenCLBuffer<InType_> OpenCLBuffer_type_in;
	typedef opencl::OpenCLBuffer<OutType_> OpenCLBuffer_type_out;
	typedef typename OpenCLBuffer_type_out::vector_type results_vector_type;

	void set_event_callback(std::function<void ()> callbackFn);
	void wait_for_completion();
	const results_vector_type& get_results() const;

private:
	template < class SrcInType_ >
	Execution(opencl::CLDevice* device, const std::vector<SrcInType_>& srcData);
	template < class SrcInType_ >
	Execution(opencl::CLDevice* device, const SrcInType_* srcData, size_t srcDataSize);
	Execution(const Execution& other);

	opencl::CLDevice* _device;
	opencl::CLProgram::CLExecution _execution;
	OpenCLBuffer_type_in _sourceData;
	mutable OpenCLBuffer_type_out _results;
	mutable bool _resultsRetieved;
};

/*

Number of octaves

Specify a noise modifier for each octave.
default: total = total + noisep3(vxyz * frequency, p, ms_grad4) * amplitude;

Specify a frequency modifier for each octave.
default: frequency = frequency * lacunarity;

Specify an amplitude modifier for each octave.
default: amplitude = amplitude * persistence;

*/

// basic noise generator, always returns the same value for a given input
template < class FloatTy_ >
struct Noise
{
	typedef FloatTy_ real_type;
	typedef NoiseFloatTraits<FloatTy_> real_traits;
	typedef typename real_traits::cl_real_type cl_real_type;
	typedef typename real_traits::cl_real2_type cl_real2_type;
	typedef typename real_traits::cl_real3_type cl_real3_type;
	typedef typename real_traits::cl_real4_type cl_real4_type;

	typedef opencl::OpenCLBuffer<cl_real3_type> OpenCLBuffer_type_in;
	typedef opencl::OpenCLBuffer<cl_real_type> OpenCLBuffer_type_out;

	typedef math::Vector4<real_type> vector4_type;
	typedef std::vector<vector4_type> pts_vector_type;
	typedef typename OpenCLBuffer_type_out::vector_type results_vector_type;

	static_assert(sizeof(real_type) == sizeof(cl_real_type), "Noise<> float type sizes do not match");
	static_assert(sizeof(vector4_type) == sizeof(cl_real3_type), "Noise<> float vector type sizes do not match");

	typedef Execution< cl_real3_type, cl_real_type > NoiseExecution;
	typedef std::shared_ptr<NoiseExecution> NoiseExecutionPtr;

public:
	Noise(/*opencl::CLDevice* device*/) 
		: _device(opencl::get_gpu_device()), 
		//_scale((real_type)1), 
		_baseOffset((real_type)0), 
		_baseScale((real_type)1) {}
	//Noise(/*opencl::CLDevice* device,*/ unsigned int seed);

	//void set_scale(real_type scale);

	/*
	perlin variables:
	vxyz		: REAL3		: the vertex position
	frequency	: REAL		: the frequency multiplier
	amplitude	: REAL		: the noise value multiplier
	perlin_ms_grad4_eval variables:
	msgradval	: REAL		: the current ms_grad4 value
	frequency	: REAL		: the frequency multiplier
	amplitude	: REAL		: the noise value multiplier
	Functions:
	NOISE() : noise function
	FRACTAL_NOISE(persistence, lacunarity, octaves) : a basic fractal noise function
	*/
	struct Octave
	{
		virtual std::string code() const = 0;
	};

	typedef std::shared_ptr<Octave> OctavePtr;

	struct BasicOctave : public Octave
	{
		BasicOctave(const real_type persistence, const real_type lacunarity)
		{
			std::stringstream ss;
			ss << "total = total + NOISE() * amplitude;\n"
				"frequency = frequency * " << lacunarity << ";\n"
				"amplitude = amplitude * " << persistence << ";\n";
			_code = ss.str();
		}

		virtual std::string code() const
		{
			return	_code;
		}

	private:
		std::string _code;
	};

	void add_octave(const OctavePtr& octave, size_t count = 1);

	void init(unsigned int seed);

	NoiseExecutionPtr enqueue_work(const pts_vector_type& pts, real_type scale = (real_type)1);
	NoiseExecutionPtr enqueue_work(const vector4_type* pts, size_t ptsCount, real_type scale = (real_type)1);

private:
	Noise(const Noise& other) {}
	Noise& operator=(const Noise& other) {}

	std::string generate_perlin_code() const;

private:
	opencl::OpenCLBuffer<cl_ushort> _p;
	opencl::OpenCLBuffer<cl_real_type> _ms_grad4;

	opencl::CLDevice* _device;
	opencl::CLProgram _program;

	//real_type _scale;

	mutable boost::mutex _executionMutex;

	typedef Execution< cl_real_type, cl_real_type > GradEvalExecution;
	typedef std::shared_ptr< GradEvalExecution > GradEvalExecutionPtr;

	//GradEvalExecutionPtr _ms_gradEval;
	//OpenCLBuffer_type_out _ms_gradResults;
	NoiseExecutionPtr _minMaxEval;
	//opencl::CLProgram::CLExecution _ms_gradExecution;
	cl_real_type _baseOffset, _baseScale;

	bool _minMaxEvaled;

	std::vector<OctavePtr> _octaves;
//	// 3d noise function
//	value_type operator()(value_type x, value_type y, value_type z) const;
//
//	// 2d noise function
//	value_type operator()(value_type x, value_type y) const;
//
//private:
//	static inline value_type fade(value_type t);
//	static inline value_type lerp(value_type t, value_type a, value_type b);
};

};

#include "perlin.inl"

#endif // perlin_h__
