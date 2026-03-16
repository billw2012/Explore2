
#include "databuilderthread.h"

namespace explore2 {;


std::shared_ptr<DataDistributeThread> DataDistributeThread::_globalInstance;

DataBuilderThread::DataBuilderThread() 
: _finished(), _totalWorkLeft(0L)
{
	// gotta make absolutely certain thread is started after the finished flag is set to false
	_buildThread.reset(new boost::thread(std::bind(&DataBuilderThread::data_building_thread, this)));
}

DataBuilderThread::~DataBuilderThread() 
{
	_finished.set();
	if(_buildThread->joinable())
		_buildThread->join();
}

void DataBuilderThread::queue_data(const BuildThreadData::ptr& data)
{
	boost::mutex::scoped_lock lockFn(_toBuildMutex);
	//chunk->building.try_lock();
	//chunk->finishedBuild.reset();
	_totalWorkLeft.add((LONG)data->get_total_work());
	_toBuild.push_back(data);
	_queueHasData.set();
}

void DataBuilderThread::data_building_thread()
{
	bool done = false;
	while(!_finished.is_set())
	{
		_queueHasData.wait_for_set(100); // allow the loop to cycle so the thread can end if it needs to
		BuildThreadData::ptr nextData = get_next_data_from_queue();
		if(nextData)
		{
			build_data(nextData);
		}
		//else done = true;
	}
}

void DataBuilderThread::build_data(BuildThreadData::ptr data)
{
	int total = (int)data->get_total_work();
	for(size_t idx = 0; idx < data->get_process_step_count() && !data->is_aborted(); ++idx)
		data->process(idx);
	_totalWorkLeft.add(-total);

	data->set_finished_build();
}

BuildThreadData::ptr DataBuilderThread::get_next_data_from_queue()
{
	boost::mutex::scoped_lock lockFn(_toBuildMutex);

	if(_toBuild.size())
	{
		BuildThreadData::ptr retVal = _toBuild.front();
		_toBuild.pop_front();
		//if(_toBuild.size() == 0)
		//	_queueHasData.reset();
		return retVal;
	}
	_queueHasData.reset();
	return NULL;
}

int DataBuilderThread::get_data_queued() const
{
	boost::mutex::scoped_lock lockFn(_toBuildMutex);
	return (int)_toBuild.size();
}

int DataBuilderThread::get_total_work_left() const
{
	return (LONG)_totalWorkLeft;
}

void DataBuilderThread::check_for_aborts()
{
	boost::mutex::scoped_lock lockFn(_toBuildMutex);

	for(size_t idx = 0; idx < _toBuild.size(); ++idx)
	{
		const BuildThreadData::ptr& data = _toBuild[idx];
		if(data->is_aborted())
		{
			data->set_finished_build();
			_totalWorkLeft.add(-static_cast<int>(data->get_work_remaining()));
			_toBuild.erase(_toBuild.begin() + idx);
			--idx;
		}
	}
}

DataDistributeThread::DataDistributeThread() 
//: _quitDistributeThread(false)
{
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	_builderThreads.push_back(DataBuilderThread::ptr(new DataBuilderThread()));
	//_distributeThread.reset(new boost::thread(std::bind(&DataDistributeThread::chunk_distribute_thread, this)));
}
//
//DataDistributeThread::~DataDistributeThread() 
//{
//	//_quitDistributeThread.set(true);
//	//if(_distributeThread->joinable())
//	//	_distributeThread->join();
//}

void DataDistributeThread::queue_data(const BuildThreadData::ptr& data)
{
	pass_out_data(data);
	//boost::mutex::scoped_lock lockFn(_toDistributeMutex);

	//_toDistribute.push_back(data);
	//_queueHasData.set();
}

void DataDistributeThread::check_for_aborts()
{
	for(size_t idx = 0; idx < _builderThreads.size(); ++idx)
	{
		_builderThreads[idx]->check_for_aborts();
	}
}

//ChunkLODThreadData* DataDistributeThread::get_next_chunk_from_queue() 
//{
//	boost::mutex::scoped_lock lockFn(_toDistributeMutex);
//
//	if(_toDistribute.size())
//	{
//		ChunkLODThreadData* retVal = _toDistribute.front();
//		_toDistribute.pop_front();
//		return retVal;
//	}
//	_queueHasData.reset();
//	return NULL;
//}

void DataDistributeThread::pass_out_data(const BuildThreadData::ptr& data)
{
	//// if we have got a spare thread slot, then create the thread
	//if(_builderThreads.size() < _maxBuilderThreads)
	//{
	//	ChunkBuilderThread::ptr newBuilderThread(new ChunkBuilderThread(chunk));
	//}
	DataBuilderThread::ptr bestBuilderThread;
	assert(_builderThreads.size() > 0);
	bestBuilderThread = _builderThreads[0];
	for(size_t idx = 1; idx < _builderThreads.size(); ++idx)
	{
		if(_builderThreads[idx]->get_total_work_left() < bestBuilderThread->get_total_work_left())
			bestBuilderThread = _builderThreads[idx];
	}
	bestBuilderThread->queue_data(data);
}

void BuildThreadData::set_abort_build()
{
	_abortBuild.set();
	DataDistributeThread::get_instance()->check_for_aborts();
}

bool BuildThreadData::is_finished_build() const
{
	return _finishedBuild.is_set();
	//return (bool)finishedBuild;
	//boost::unique_lock<boost::mutex> tryLock(building, boost::try_to_lock_t());
	//return !tryLock.owns_lock();
}

bool BuildThreadData::is_aborted() const
{
	return _abortBuild.is_set();
}

void BuildThreadData::wait_for_build()
{
	//while(!finishedBuild) { Sleep(0); }

	_finishedBuild.wait_for_set();
}

void BuildThreadData::reset_flags()
{
	_abortBuild.reset();
	_finishedBuild.reset();
}

void BuildThreadData::set_finished_build()
{
	_finishedBuild.set();
	//finishedBuild = true;
}

}