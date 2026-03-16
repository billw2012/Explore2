
#include "streamingmanager.h"
#include "Misc/functions.hpp"

namespace glbase {;


void StreamableInterface::stream_now( size_t start /*= 0*/, size_t count /*= 0*/ )
{
	// if we are streaming everything now then cancel all pending streaming for this object
	if(start == 0 && count == 0)
		StreamingManager::cancel(this);
	if(count == 0) count = get_size_bytes();
	load_data(start, count);
}

void StreamableInterface::stream_later( int priority /*= 5*/, size_t start /*= 0*/, size_t count /*= 0*/ )
{
	// if count is 0 then stream everything
	if(count == 0) count = get_size_bytes();
	StreamingManager::add(this, priority, start, count);
}

bool StreamableInterface::is_ready()
{
	return _activeStreamingRequests == 0;
}

void StreamableInterface::cancel_streaming( /*size_t start*/ /*= 0*//*, size_t count*/ /*= 0*/ )
{
	StreamingManager::cancel(this);
}

std::shared_ptr<void*> StreamingManager::init( size_t bytesPerFrame, size_t allowedOverrun )
{
	_bytesPerFrame = bytesPerFrame;
	_allowedOverrun = allowedOverrun;

	return std::shared_ptr<void*>(nullptr, [](void*) { StreamingManager::shutdown(); });
}

void StreamingManager::shutdown()
{
	_streamingList.clear();
}

void StreamingManager::add( StreamableInterface* data, int priority, size_t start, size_t count )
{
	_streamingList.insert(std::make_pair(StreamCommandKey(priority, ++_currIndex), StreamCommandData(data, start, count)));
}

void StreamingManager::cancel( StreamableInterface* data/*, size_t start, size_t count*/ )
{
	misc::erase_if(_streamingList, [&data](const std::map<StreamCommandKey, StreamCommandData>::value_type& keyDataPair)
	{
		return keyDataPair.second.src == data;
	});
}

void StreamingManager::process()
{
	size_t bytesRemaining = _bytesPerFrame;

	while(bytesRemaining > 0 && !_streamingList.empty())
	{
		StreamCommandData& data = _streamingList.begin()->second;
		size_t toTransfer;
		// if we could stream without overrun
		if(data.count <= bytesRemaining)
		{
			bytesRemaining -= data.count;
			toTransfer = data.count;
		}
		// if we could stream with overrun
		else if(data.count <= bytesRemaining + _allowedOverrun)
		{
			bytesRemaining = 0;
			toTransfer = data.count;
		}
		// we can only partially stream
		else
		{
			toTransfer = bytesRemaining;
			bytesRemaining = 0;
		}
		data.src->load_data(data.start, toTransfer);
		data.start += toTransfer;
		data.count -= toTransfer;
		if(data.count == 0)
		{
			_streamingList.erase(_streamingList.begin());
		}
	}
}

void StreamingManager::complete()
{
	for(auto itr = _streamingList.begin(); itr != _streamingList.end(); ++itr)
	{
		StreamCommandData& data = itr->second;
		data.src->load_data(data.start, data.count);
	}
	_streamingList.clear();
}

bool StreamingManager::is_empty()
{
	return _streamingList.empty();
}

size_t StreamingManager::_currIndex = 0;
size_t StreamingManager::_allowedOverrun;
size_t StreamingManager::_bytesPerFrame;
std::map<StreamingManager::StreamCommandKey, StreamingManager::StreamCommandData> StreamingManager::_streamingList;

}