#ifndef OpenCLWrappers_h__
#define OpenCLWrappers_h__


#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <memory>
#include <functional>

#include "math/vector3.hpp"
#include "CL/opencl.h"
#include "CL/cl_ext.h"

#include "Utils/stl_aligned_allocator.h"

namespace opencl {;

struct OpenCL;

struct CLDevice
{
	CLDevice() {}
	CLDevice(cl_device_id device_, cl_context context_, cl_command_queue queue_);

	void flush() const { ::clFlush(queue); }

	void wait_for_all() const { ::clFinish(queue); }

	void enqueue_barrier();

	cl_command_queue queue;
	cl_device_id device;

	bool available;
	bool compilerAvailable;
	cl_ulong memCacheSize;
	cl_ulong memSize;
	cl_uint clockFrequency;
	cl_uint computeUnits;
	std::string name;
	std::string profile;
	cl_device_type type;
	std::string vendor;
	std::string deviceVersion;
	std::string driverVersion;
	cl_device_fp_config doubleSupport;
	std::string extensions;
	size_t maxWorkGroupSize;
	cl_uint maxWorkItemDimensions;
	std::vector<size_t> maxWorkItemSizes;
	cl_uint maxSubDevices;
	std::vector<cl_device_partition_property_ext> subDevicePartitionProperties;
	cl_context context;
};

struct CLPlatform
{
	CLPlatform() {}

	CLPlatform(cl_platform_id platform_);

	//std::vector<CLDevice> get_devices() const;

	cl_platform_id platform;
	std::string profile, version, name, vendor, extensions;

	typedef std::map<cl_device_id, CLDevice> DeviceMap;
	DeviceMap devices;
};

bool init();
cl_context get_context();
CLDevice* get_gpu_device();
const std::vector<CLDevice>& get_all_devices();


struct CLPlainEvent
{
	CLPlainEvent(cl_event event_) : event(event_) {}

	cl_event event;
	cl_event get_event() const { return event; }
};

struct CLEventSet
{
	virtual bool clear_complete();
	virtual bool commands_complete();
	virtual void wait_all();

protected:

	void add_event(cl_event event);

private:

	std::vector<cl_event> _activeCommands;
};

template < class ValTy_ >
struct OpenCLBuffer : public CLEventSet
{
	typedef ValTy_ value_type;

	typedef std::vector< value_type, aligned_allocator<value_type, 8> > vector_type;

	struct Flags { enum type {
		KERNEL_READ			= 1 << 0,
		KERNEL_WRITE		= 1 << 1,
		KERNEL_READ_WRITE	= KERNEL_READ + KERNEL_WRITE
	};};

	OpenCLBuffer();
	~OpenCLBuffer();

	bool init(typename Flags::type flags);

	bool resize(size_t elements);
	void destroy();

	cl_event enqueue_read(const CLDevice& device, bool blocking = false);
	cl_event enqueue_read(const CLDevice& device, bool blocking, size_t elementOffset,
		size_t elementCount);
	cl_event enqueue_write(const CLDevice& device, bool blocking = false);
	cl_event enqueue_write(const CLDevice& device, bool blocking, size_t elementOffset,
		size_t elementCount);

	const value_type& operator[](size_t idx) const;
	value_type& operator[](size_t idx);
	const vector_type& get_data() const;
	vector_type& get_data();
	size_t size() const;

	template < class Ty_ >
	const Ty_& get_as(size_t idx) const;
	template < class Ty_ >
	Ty_& get_as(size_t idx);

	cl_mem get_handle() const;


private:

	cl_mem_flags interpret_flags(typename Flags::type flags);

	cl_mem _handle;
	cl_context _context;
	cl_mem_flags _flags;
	cl_int _lastError;
	vector_type _data;
};

struct CLProgram : public CLEventSet
{
	struct ProgramOptions
	{
		ProgramOptions();
		ProgramOptions& add_include_dir(const std::string& includeDir);
		ProgramOptions& add_macro(const std::string& name, const std::string& value = std::string());
		ProgramOptions& debugging();

		const char* c_str() const;
	private:
		std::string _optionsString;
	};
	CLProgram();

	bool is_valid() const { return _program != NULL; }

	bool create_from_file(const std::string& path, const ProgramOptions& compilerOptions, const CLDevice& device);
	bool create(const std::string& source, const ProgramOptions& compilerOptions, const CLDevice& device);
	bool create(const std::vector<std::string>& sourcelines, const ProgramOptions& compilerOptions, const CLDevice& device);
	bool create_from_file(const std::string& path, const ProgramOptions& compilerOptions, const std::vector<CLDevice>& devices = std::vector<CLDevice>());
	bool create(const std::string& source, const ProgramOptions& compilerOptions, const std::vector<CLDevice>& devices = std::vector<CLDevice>());
	bool create(const std::vector<std::string>& sourcelines, const ProgramOptions& compilerOptions, const std::vector<CLDevice>& devices = std::vector<CLDevice>());

	std::string get_build_log(const CLDevice& device) const;

	bool create_kernal(const std::string& kernelFnName);
//#if defined(CL_VERSION_1_2)
	template < class ValTy_ > 
	bool bind_parameter(const std::string& kernelFnName, const std::string& paramName, const ValTy_& val) const;
	template < class ValTy_ > 
	bool bind_parameter(const std::string& kernelFnName, const std::string& paramName, const OpenCLBuffer<ValTy_>& buffer) const;
//#endif
#if !defined(CL_VERSION_1_2)
	void add_kernel_param(const std::string& kernelFnName, const std::string& paramName);
	void specify_kernel_param_index(const std::string& kernelFnName, const std::string& paramName, cl_uint paramIdx);
#endif

	template < class ValTy_ > 
	bool bind_parameter(const std::string& kernelFnName, cl_uint paramIdx, const ValTy_& val) const;
	template < class ValTy_ > 
	bool bind_parameter(const std::string& kernelFnName, cl_uint paramIdx, const OpenCLBuffer<ValTy_>& buffer) const;


	struct CLExecution
	{
		CLExecution(size_t globalWorkOffset_= 0, size_t globalWorkSize_= 0, size_t localWorkSize_ = 0)
			: globalWorkOffset(globalWorkOffset_), globalWorkSize(globalWorkSize_), localWorkSize(localWorkSize_), event(NULL) {}
		
		bool is_valid() const 
		{
			return event != NULL;
		}
		
		void wait() const
		{
			::clWaitForEvents(1, &event);
		}

		void set_event_callback(std::function<void ()> callbackFn);

		cl_event event;
		size_t globalWorkOffset; 
		size_t globalWorkSize; 
		size_t localWorkSize; 
	};

	//CLExecution enqueue_work(const CLDevice& device, const std::string& kernelFnName, size_t globalWorkSize, size_t localWorkSize);
	CLExecution enqueue_work(const CLDevice& device, const std::string& kernelFnName, size_t workSize) const;

protected:

private:

	cl_context _context;
	cl_program _program;
	mutable cl_int _lastError;

	struct CLKernel
	{
		typedef std::map<std::string, cl_uint> ArgumentMap;
		ArgumentMap arguments;
		cl_kernel kernel;
	};

	typedef std::map<std::string, CLKernel> KernalMap;
	KernalMap _kernals;	
};


template < class ValTy_ >
typename opencl::OpenCLBuffer<ValTy_>::Flags::type operator|(const typename opencl::OpenCLBuffer<ValTy_>::Flags::type& lhs, const typename opencl::OpenCLBuffer<ValTy_>::Flags::type& rhs)
{
	return static_cast<typename opencl::OpenCLBuffer<ValTy_>::Flags::type>(static_cast<size_t>(lhs) | static_cast<size_t>(rhs));
}

inline cl_double3 convert_my_vec3(const math::Vector3<double>& myVec)
{
	cl_double3 tv = {myVec.x, myVec.y, myVec.z, 0.0};
	return tv;
}

inline cl_float3 convert_my_vec3(const math::Vector3<float>& myVec)
{
	cl_float3 tv = {myVec.x, myVec.y, myVec.z, 0.0f};
	return tv;
}

inline math::Vector3<double> convert_to_my_vec3(const cl_double3& vec)
{
	return math::Vector3<double>(vec.s[0], vec.s[1], vec.s[2]);
}

inline math::Vector3<float> convert_to_my_vec3(const cl_float3& vec)
{
	return math::Vector3<float>(vec.s[0], vec.s[1], vec.s[2]);
}

};

#include "OpenCLWrappers.inl"


#endif // OpenCLWrappers_h__