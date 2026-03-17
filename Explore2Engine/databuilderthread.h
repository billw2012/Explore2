#if !defined(__EXPLORE2_DATA_BUILDER_THREAD_H__)
#define __EXPLORE2_DATA_BUILDER_THREAD_H__

#include <vector>
#include <deque>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include "Misc/atomic.hpp"

#include "Misc/waithandle.hpp"

namespace explore2 {;

struct BuildThreadData
{
	typedef std::shared_ptr< BuildThreadData > ptr;

	BuildThreadData(size_t processSteps = 1) : _processSteps(processSteps) {}

	void set_finished_build();
	bool is_finished_build() const;
	void set_abort_build();
	bool is_aborted() const;

	void wait_for_build();

	size_t get_process_step_count() const { return _processSteps; }

	virtual void process(size_t step) = 0;
	virtual size_t get_total_work() const = 0;
	virtual size_t get_work_remaining() const = 0;

	void reset_flags();

	BuildThreadData(const BuildThreadData& from)
		: _processSteps(from._processSteps) {}

	void operator=(const BuildThreadData& from) 
	{
		_processSteps = from._processSteps;
		_abortBuild.reset();
		_finishedBuild.reset();
	}

private:

private: // data

	misc::WaitHandle _abortBuild;
	misc::WaitHandle _finishedBuild;
	size_t _processSteps;
};


struct DataBuilderThread
{
	typedef std::deque<BuildThreadData::ptr> BuildThreadDataDeque;

	typedef std::shared_ptr<DataBuilderThread> ptr;

	DataBuilderThread();
	~DataBuilderThread();

	void queue_data(const BuildThreadData::ptr& data);

	BuildThreadData::ptr get_next_data_from_queue();

	int get_data_queued() const;

	int get_total_work_left() const; 

	void check_for_aborts();

private:
	void data_building_thread();

	void build_data(BuildThreadData::ptr data);

private:
	mutable boost::mutex _toBuildMutex;
	BuildThreadDataDeque _toBuild;
	misc::AtomicInt32 _totalWorkLeft;
	misc::WaitHandle _finished;
	std::shared_ptr<boost::thread> _buildThread;

	misc::WaitHandle _queueHasData;
};

struct DataDistributeThread
{
	DataDistributeThread();

	void queue_data(const BuildThreadData::ptr& data);

	static DataDistributeThread* get_instance()
	{
		if(!_globalInstance)
			_globalInstance.reset(new DataDistributeThread());
		return _globalInstance.get();
	}

	static void destroy()
	{
		_globalInstance.reset();
	}

	struct DataBuilderThreadStats
	{
		DataBuilderThreadStats(size_t dataQueued_, size_t totalWorkLeft_) 
			: dataQueued(dataQueued_), totalWorkLeft(totalWorkLeft_) {}
		size_t dataQueued;
		size_t totalWorkLeft;
	};

	std::vector<DataBuilderThreadStats> get_builder_thread_data() const 
	{
		std::vector<DataBuilderThreadStats> data;

		for(size_t idx = 0; idx < _builderThreads.size(); ++idx)
		{
			DataBuilderThread::ptr bt = _builderThreads[idx];
			data.push_back(DataBuilderThreadStats(bt->get_data_queued(), 
				bt->get_total_work_left()));
		}

		return data;
	}

	void check_for_aborts();

private:
//	void data_distribute_thread();
//	BuildThreadData::ptr get_next_data_from_queue();
	void pass_out_data(const BuildThreadData::ptr& data);

private:
	static std::shared_ptr<DataDistributeThread> _globalInstance;

	boost::mutex _toDistributeMutex, _builderThreadsMutex;
	DataBuilderThread::BuildThreadDataDeque _toDistribute;
	std::vector< DataBuilderThread::ptr > _builderThreads;
	misc::AtomicInt32 _quitDistributeThread, _maxBuilderThreads;

	std::shared_ptr<boost::thread> _distributeThread;

	misc::WaitHandle _queueHasChunks;
};

}

#endif // __EXPLORE2_DATA_BUILDER_THREAD_H__