#ifndef Profiler_h__
#define Profiler_h__

#include "HRTimer.h"
#include <string>
#include <memory>

namespace utils {;

struct ProfileBlock
{
	ProfileBlock(const std::string& blockName, size_t historyLen, bool ignore = false);
	~ProfileBlock();

	void allow() { _ignore = false; }
private:
	std::string _blockName;
	size_t _historyLen;
	HRTimer _timer;
	bool _ignore;
};

struct Profiler
{
	//struct ScopedBlock
	//{
	//	ScopedBlock(const std::string& blockName)
	//		: _blockName(blockName) 
	//	{
	//		Profiler::start_block(blockName);
	//	}
	//	~ScopedBlock()
	//	{
	//		Profiler::end_block(_blockName);
	//	}

	//private:
	//	std::string _blockName;
	//};

	//static void start_block(const std::string& blockName, size_t historyLen);
	//static void end_block(const std::string& blockName);
	//static std::shared_ptr<void> scoped_block(const std::string& blockName, size_t historyLen);
	typedef std::unique_ptr<ProfileBlock> block_handle;

	static block_handle start_block(const std::string& blockName, size_t historyLen);
	static void end_block(block_handle& blockHandle);

	static float get_timing_ms(const std::string& blockName, size_t samples = 1);
};

}

#endif // Profiler_h__
