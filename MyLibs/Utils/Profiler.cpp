
#include "Profiler.h"
#include <deque>
#include <unordered_map>
#include <boost/thread/mutex.hpp>

namespace utils {;

struct Profile
{
	Profile() : historyLen(1) {}

	std::deque<unsigned int> timeHistory;
	size_t historyLen;
};

boost::mutex _profilesMutex;
std::unordered_map<std::string, Profile> _profiles;

//ProfileBlock Profiler::start_block( size_t historyLen )
//{
//	Profile& profile = _profiles[blockName];
//	profile.historyLen = historyLen;
//	profile.timer.reset();
//}
//
//void Profiler::end_block( const std::string& blockName )
//{
//	Profile& profile = _profiles[blockName];
//	profile.timeHistory.push_back(profile.timer.get_time());
//	if(profile.timeHistory.size() > profile.historyLen)
//	{
//		profile.timeHistory.pop_front();
//	}
//}

//std::shared_ptr<void> Profiler::scoped_block( const std::string& blockName, size_t historyLen )
//{
//	start_block(blockName, historyLen);
//	return std::shared_ptr<void>(nullptr, [blockName](void*) { end_block(blockName); });
//}
 
Profiler::block_handle Profiler::start_block(const std::string& blockName, size_t historyLen)
{
	return Profiler::block_handle(new ProfileBlock(blockName, historyLen, true));
}

void Profiler::end_block(block_handle& blockHandle)
{
	blockHandle->allow();
	blockHandle.reset();
}

float Profiler::get_timing_ms( const std::string& blockName, size_t samples )
{
	boost::mutex::scoped_lock lock(_profilesMutex);

	const Profile& profile = _profiles[blockName];

	float totalTime = 0.0f;
	samples = std::min<size_t>(samples, profile.timeHistory.size());
	if(samples == 0.0f)
		return 0.0f;

	auto itr = profile.timeHistory.rbegin();
	for(size_t idx = 0; idx < samples; ++idx, ++itr)
	{
		totalTime += static_cast<float>(*itr);
	}
	totalTime /= samples;
	return totalTime;
}


ProfileBlock::ProfileBlock( const std::string& blockName, size_t historyLen, bool ignore /*= false*/ ) 
	: _blockName(blockName),
	_historyLen(historyLen),
	_ignore(ignore)
{

}

ProfileBlock::~ProfileBlock()
{
	if(!_ignore)
	{
		boost::mutex::scoped_lock lock(_profilesMutex);
		Profile& profile = _profiles[_blockName];
		profile.timeHistory.push_back(_timer.get_time());
		if(profile.timeHistory.size() > _historyLen)
		{
			profile.timeHistory.pop_front();
		}
	}
}

}