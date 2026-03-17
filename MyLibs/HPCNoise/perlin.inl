
// #include <boost/random/uniform_real_distribution.hpp>
// #include <boost/random/uniform_int_distribution.hpp>
// #include <boost/random/taus88.hpp>
// #include <boost/random/seed_seq.hpp>
#include <random>

namespace HPCNoise {;

// template < class FloatTy_ >
// Noise<FloatTy_>::Noise(unsigned int seed) : _device(opencl::get_gpu_device()), _scale((real_type)1)
// {
// 	init(seed);
// }

template < class FloatTy_ >
void Noise<FloatTy_>::add_octave(const OctavePtr& octave, size_t count)
{
	for(size_t idx = 0; idx < count; ++idx)
		_octaves.push_back(octave);
}


template < class FloatTy_ >
std::string Noise<FloatTy_>::generate_perlin_code() const
{
	const std::vector<std::string>& baseCodeParts = internal::get_shader_base();
	assert(baseCodeParts.size() == 3);

	std::stringstream fullCode;
	
	fullCode << baseCodeParts[0];
	for(auto itr = _octaves.begin(); itr != _octaves.end(); ++itr)
	{
		std::string code = (*itr)->code();
		fullCode << code;
	}
	fullCode << baseCodeParts[1];
	for(auto itr = _octaves.begin(); itr != _octaves.end(); ++itr)
	{
		std::string code = (*itr)->code();
		fullCode << code;
	}
	fullCode << baseCodeParts[2];

	return fullCode.str();
}


template < class FloatTy_ >
void Noise<FloatTy_>::init(unsigned int seed)
{
	std::default_random_engine rand;
	rand.seed(seed + 1); // can't be zero... apparently...
	std::uniform_int_distribution<int> dist(0, 255);

	_p.init(opencl::OpenCLBuffer<cl_ushort>::Flags::KERNEL_READ);
	_p.resize(512);

	opencl::OpenCLBuffer<cl_ushort>::vector_type& p_data = _p.get_data();

	// p is a random index table indexing from 0 to 255
	// create p
	for (int i = 0; i < 256; ++i) 
		p_data[256+i] = p_data[i] = (cl_ushort)dist(rand) % 255; //(rand() % 255);

	// kkf is a sequence of floats from -1 to 1
	float kkf[256];
	for (int i = 0; i < 256; ++i)
		kkf[i] = -1.0f + 2.0f * ((float)i / 255.0f);

	_ms_grad4.init(opencl::OpenCLBuffer<real_type>::Flags::KERNEL_READ);
	_ms_grad4.resize(512);

	opencl::OpenCLBuffer<real_type>::vector_type& ms_grad4_data = _ms_grad4.get_data();

	// sPMul is 1/Sqrt(2)
	static real_type sPMul = (real_type)(1.0/1.4142135623730950488016887242097);

	// ms_grad4 is a sequence of random numbers from -1/Sqrt(2) to 1/Sqrt(2)
	for (int i = 0; i < 256; i++)
		ms_grad4_data[i] = kkf[p_data[i]] /** sPMul*/;

	for (int i = 256; i < 512; i++)
		ms_grad4_data[i] = ms_grad4_data[i & 255]; 

	opencl::CLProgram::ProgramOptions programOptions;
	if(sizeof(real_type) == sizeof(double))
		programOptions.add_macro("USE_DOUBLE_PRECISION");

	if(!_program.create(generate_perlin_code(), programOptions, *_device))
	{
		std::cout << _program.get_build_log(*_device) << std::endl;
		return ;
	}

	//if(!_program.create_kernal("perlin_ms_grad4_eval"))
	//{
	//	std::cout << _program.get_build_log(*_device) << std::endl;
	//	return ;
	//}


	//_program.bind_parameter("perlin_ms_grad4_eval", "ms_grad4", _ms_grad4);
	//boost::mutex::scoped_lock lockScope(_executionMutex);

	if(!_program.create_kernal("perlin"))
	{
		std::cout << _program.get_build_log(*_device) << std::endl;
		return ;
	}
#if !defined(CL_VERSION_1_2)
	// add the kernel parameters in order they appear
	_program.add_kernel_param("perlin", "inPositions");
	_program.add_kernel_param("perlin", "outResults");
	_program.add_kernel_param("perlin", "offset");
	_program.add_kernel_param("perlin", "scale");
	_program.add_kernel_param("perlin", "p");
	_program.add_kernel_param("perlin", "ms_grad4");
#endif
	_program.bind_parameter("perlin", "p", _p);
	_program.bind_parameter("perlin", "ms_grad4", _ms_grad4);

	_p.enqueue_write(*_device);
	_ms_grad4.enqueue_write(*_device);

	//_ms_gradResults.init(OpenCLBuffer_type_out::Flags::KERNEL_WRITE);
	//_ms_gradResults.resize(_ms_grad4.size());
	//_program.bind_parameter("perlin", "inPositions", execution->_sourceData);
	//_program.bind_parameter("perlin", "outResults", execution->_results);
	//_program.bind_parameter("perlin", "offset", 0);
	//_program.bind_parameter("perlin", "scale", 1.0);
	//_program.bind_parameter("perlin", "outResults", _ms_gradResults);
	//_program.bind_parameter("perlin_ms_grad4_eval", "lacunarity", (cl_real_type)3.0f);
	//_program.bind_parameter("perlin_ms_grad4_eval", "persistence", (cl_real_type)0.5f);
	//_ms_gradExecution = _program.enqueue_work(*_device, "perlin_ms_grad4_eval", _ms_grad4.size());

	std::uniform_real_distribution<real_type> distf(-1.0, 1.0);
	pts_vector_type minMaxPts(256);
	for(size_t idx = 0; idx < 256; ++idx)
	{
		minMaxPts.push_back(vector4_type(distf(rand), distf(rand), distf(rand), (real_type)0.0));
	}
	_minMaxEvaled = true;
	_minMaxEval = enqueue_work(minMaxPts);
	_minMaxEvaled = false;

	std::cout << _program.get_build_log(*_device) << std::endl;
}


// template < class FloatTy_ >
// void Noise<FloatTy_>::set_scale( real_type scale )
// {
// 	_scale = scale;
// 	//if(_program.is_valid())
// 	//	_program.bind_parameter("perlin", "scale", _scale * _baseScale);
// }

template < class FloatTy_ >
typename Noise<FloatTy_>::NoiseExecutionPtr Noise<FloatTy_>::enqueue_work(const vector4_type* pts, size_t ptsCount, real_type scale /*= (real_type)1*/)
	//opencl::CLProgram::CLExecution Noise<FloatTy_>::enqueue_work(const src_data_type& pts) const
{
	boost::mutex::scoped_lock lockScope(_executionMutex);

	if(!_minMaxEvaled)
	{
		//_ms_gradResults.enqueue_read(*_device, true);
		//const OpenCLBuffer_type_out::vector_type& ms_gradEvalResults = _ms_gradResults.get_data();
		const OpenCLBuffer_type_out::vector_type& ms_gradEvalResults = _minMaxEval->get_results();
		auto minMaxItrs = std::minmax_element(ms_gradEvalResults.begin(), ms_gradEvalResults.end());
		cl_real_type baseMin = *(minMaxItrs.first);
		cl_real_type baseMax = *(minMaxItrs.second);
		_baseOffset = -(baseMin + baseMax) * (cl_real_type)0.5;
		_baseScale = 2 / (baseMax - baseMin);
		_minMaxEvaled = true;
	}

	NoiseExecutionPtr execution(new NoiseExecution(_device, pts, ptsCount));
	_program.bind_parameter("perlin", "inPositions", execution->_sourceData);
	_program.bind_parameter("perlin", "outResults", execution->_results);
	_program.bind_parameter("perlin", "offset", _baseOffset);
	_program.bind_parameter("perlin", "scale", scale * _baseScale);
	//_program.bind_parameter("perlin", "lacunarity", (cl_real_type)3.0f);
	//_program.bind_parameter("perlin", "persistence", (cl_real_type)0.5f);
	//_program.bind_parameter("perlin", "scale", (cl_real_type)_scale);
	execution->_execution = _program.enqueue_work(*_device, "perlin", ptsCount);
	return execution;
}

template < class FloatTy_ >
typename Noise<FloatTy_>::NoiseExecutionPtr Noise<FloatTy_>::enqueue_work(const pts_vector_type& pts, real_type scale /*= (real_type)1*/)
//opencl::CLProgram::CLExecution Noise<FloatTy_>::enqueue_work(const src_data_type& pts) const
{
	return enqueue_work(&pts[0], pts.size(), scale);
	//boost::mutex::scoped_lock lockScope(_executionMutex);

	//if(!_minMaxEvaled)
	//{
	//	//_ms_gradResults.enqueue_read(*_device, true);
	//	//const OpenCLBuffer_type_out::vector_type& ms_gradEvalResults = _ms_gradResults.get_data();
	//	const OpenCLBuffer_type_out::vector_type& ms_gradEvalResults = _minMaxEval->get_results();
	//	auto minMaxItrs = std::minmax_element(ms_gradEvalResults.begin(), ms_gradEvalResults.end());
	//	cl_real_type baseMin = *(minMaxItrs.first);
	//	cl_real_type baseMax = *(minMaxItrs.second);
	//	_baseOffset = -(baseMin + baseMax) * (cl_real_type)0.5;
	//	_baseScale = 2 / (baseMax - baseMin);
	//	_minMaxEvaled = true;
	//}

	//NoiseExecutionPtr execution(new NoiseExecution(_device, pts));
	//_program.bind_parameter("perlin", "inPositions", execution->_sourceData);
	//_program.bind_parameter("perlin", "outResults", execution->_results);
	//_program.bind_parameter("perlin", "offset", _baseOffset);
	//_program.bind_parameter("perlin", "scale", scale * _baseScale);
	////_program.bind_parameter("perlin", "lacunarity", (cl_real_type)3.0f);
	////_program.bind_parameter("perlin", "persistence", (cl_real_type)0.5f);
	////_program.bind_parameter("perlin", "scale", (cl_real_type)_scale);
	//execution->_execution = _program.enqueue_work(*_device, "perlin", pts.size());
	//return execution;
}

template < class InType_, class OutType_ >
template < class SrcInType_ >
Execution< InType_, OutType_ >::Execution(opencl::CLDevice* device, const std::vector<SrcInType_>& srcData)
	: _device(device), _resultsRetieved(false)
{
	static_assert(sizeof(InType_) == sizeof(SrcInType_), "HPCNoise::Execution type size doesn't match.");

	_sourceData.init(OpenCLBuffer_type_in::Flags::KERNEL_READ);
	_results.init(OpenCLBuffer_type_out::Flags::KERNEL_WRITE);
	_sourceData.resize(srcData.size());
	OpenCLBuffer_type_in::vector_type& indata = _sourceData.get_data();
	memcpy_s(&indata[0], sizeof(InType_) * indata.size(), &srcData[0], sizeof(SrcInType_) * srcData.size());
	_sourceData.enqueue_write(*_device);
	_results.resize(srcData.size());
}

template < class InType_, class OutType_ >
template < class SrcInType_ >
Execution< InType_, OutType_ >::Execution(opencl::CLDevice* device, const SrcInType_* srcData, size_t srcDataSize)
	: _device(device), _resultsRetieved(false)
{
	static_assert(sizeof(InType_) == sizeof(SrcInType_), "HPCNoise::Execution type size doesn't match.");

	_sourceData.init(OpenCLBuffer_type_in::Flags::KERNEL_READ);
	_results.init(OpenCLBuffer_type_out::Flags::KERNEL_WRITE);
	_sourceData.resize(srcDataSize);
	OpenCLBuffer_type_in::vector_type& indata = _sourceData.get_data();
	memcpy_s(&indata[0], sizeof(InType_) * indata.size(), srcData, sizeof(SrcInType_) * srcDataSize);
	_sourceData.enqueue_write(*_device);
	_results.resize(srcDataSize);
}

template < class InType_, class OutType_ >
void Execution< InType_, OutType_ >::wait_for_completion()
{
	_execution.wait();
}

template < class InType_, class OutType_ >
void Execution< InType_, OutType_ >::set_event_callback(std::function<void ()> callbackFn)
{
	_execution.set_event_callback(callbackFn);
}

template < class InType_, class OutType_ >
const typename Execution< InType_, OutType_ >::results_vector_type& Execution< InType_, OutType_ >::get_results() const
{
	if(!_resultsRetieved)
	{
		_results.enqueue_read(*_device, true);		
	}
	return _results.get_data();
}

}