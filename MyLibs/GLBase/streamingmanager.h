#ifndef streamingcommand_h__
#define streamingcommand_h__

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace glbase {;

struct StreamableInterface
{
	friend struct StreamingManager;

	// stream range immediately
	void stream_now(size_t start = 0, size_t count = 0);
	// queue range for streaming async
	void stream_later(int priority = 5, size_t start = 0, size_t count = 0);
	// is all pending data streamed
	bool is_ready();
	// cancel streaming for a range
	void cancel_streaming(/*size_t start = 0, size_t count = 0*/);

	virtual size_t get_size_bytes() const = 0;

protected:

	// is the buffer splitable, or must it be loaded in one go?
	virtual bool is_splitable() const = 0;
	// callback for streaming manager to stream data
	virtual size_t load_data(size_t start, size_t count) = 0;
	// get a name that can be used to identify the streaming data for profiling etc.
	virtual std::string get_debug_name() const = 0;

private:

	// number of pending streaming requests
	int _activeStreamingRequests;
};

struct StreamingManager
{
	// allowedOverrun: amount extra that can be streamed if it would complete an operation this frame
	static std::shared_ptr<void*> init(size_t bytesPerFrame, size_t allowedOverrun);
	static void shutdown();

	// add to streaming list, taking into account existing commands and ranges, might generate multiple commands with subranges
	static void add(StreamableInterface* data, int priority, size_t start, size_t count);
	// cancel all streams or just a range (not implemented)
	static void cancel(StreamableInterface* data/*, size_t start, size_t count*/);
	// perform streaming operations for this frame
	static void process();
	// perform all pending streaming operations
	static void complete();
	// return true if there are no streaming operations pending
	static bool is_empty();

private:
	struct StreamCommandKey
	{
		StreamCommandKey(int priority_ = 0, size_t index_ = 0)
			: priority(priority_),
			index(index_) {}

		int priority;
		size_t index;

		bool operator<(const StreamCommandKey& other) const
		{
			if(priority < other.priority) return true;
			if(priority > other.priority) return true;
			return index < other.index;
		}
	};

	struct StreamCommandData
	{
		StreamCommandData(StreamableInterface* src_ = nullptr, size_t start_ = 0, size_t count_ = 0)
			: src(src_),
			start(start_),
			count(count_) 
		{
			++(src->_activeStreamingRequests);
		}
		~StreamCommandData()
		{
			--(src->_activeStreamingRequests);
		}

		StreamableInterface* src;
		size_t start, count;
	};

	static std::map<StreamCommandKey, StreamCommandData> _streamingList;
	static size_t _bytesPerFrame, _allowedOverrun, _currIndex; 
};

}

#endif // streamingcommand_h__
