#if !defined(__EXPLORE2_REMOTE_DEBUG_INTERFACE_H__)
#define __EXPLORE2_REMOTE_DEBUG_INTERFACE_H__

////#define BOOST_BIND_ENABLE_STDCALL


#include <boost/serialization/vector.hpp>

#include "Remote/RemoteDebug.h"

namespace explore2{;

namespace remote_utils{;

struct FrameTimeEvent
{
	//float fps;
	//size_t totalFrameTime;
	size_t frameTime;
	size_t frames;

	FrameTimeEvent(size_t frameTime_ = 0, size_t frames_ = 0) : frameTime(frameTime_), frames(frames_) {}

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & frameTime;
		ar & frames;
	}

};

struct ChunkLODStatsEvent
{
	typedef std::vector< size_t > StatesVector;

	StatesVector states;
	size_t depth;
	size_t oneWeightsCount;
	size_t zeroWeightsCount;

	ChunkLODStatsEvent(const StatesVector& states_ = StatesVector(), 
		size_t depth_ = 0, size_t oneWeightsCount_ = 0, size_t zeroWeightsCount_ = 0) 
		: states(states_), depth(depth_), oneWeightsCount(oneWeightsCount_), zeroWeightsCount(zeroWeightsCount_) {}

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & states;
		ar & depth;
		ar & oneWeightsCount;
		ar & zeroWeightsCount;
	}
};

struct TextureData
{
	std::string name;
	unsigned int id, handle, width, height, depth, type, value_type;

	TextureData() {}

	TextureData(unsigned int id_, unsigned int handle_, const std::string& name_, unsigned int type_, unsigned int width_, 
		unsigned int height_, unsigned int depth_, unsigned int value_type_)
		: id(id_),
		handle(handle_),
		name(name_),
		type(type_),
		width(width_),
		height(height_),
		depth(depth_),
		value_type(value_type_)
	{}

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & id;
		ar & handle;
		ar & name;
		ar & type;
		ar & width;
		ar & height;
		ar & depth;
		ar & value_type;
	}
};

struct TextureList
{
	std::vector<TextureData> textures;

	void add_texture(unsigned int id, unsigned int handle, const std::string& name, unsigned int type, unsigned int width, 
		unsigned int height, unsigned int depth, unsigned int value_type)
	{
		textures.push_back(TextureData(id, handle, name, type, width, height, depth, value_type));
	}

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & textures;
		//int count = textures.size();
		//ar & count;
		//textures.resize(count);
		//for(int idx = 0; idx < count; ++idx)
		//{
		//	ar & textures[idx];
		//}
	}
};

struct TexturePixels
{
	int width, height, depth;
	std::vector<float> pixels;

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & width;
		ar & height;
		ar & depth;
		ar & pixels;
	}
};

struct TextureRequest
{
	unsigned int index;

	TextureRequest(unsigned int index_ = std::numeric_limits<unsigned int>::max()) : index(index_) {}

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & index;
	}
};

struct Command
{
	std::string commandStr;

	Command(const std::string& commandStr_ = std::string("invalid")) : commandStr(commandStr_) {}
	
private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & commandStr;
	}
};

////typedef std::function< void (std::string& msg) > StringEventCallbackFn;
//template < 
//	class StringEventCallbackFn,
//	class FPSEventCallbackFn,
//	class ChunkLODStatsCallbackFn,
//	class TextureListCallbackFn,
//	class TexturePixelsCallbackFn
//>
//void register_events(StringEventCallbackFn		stringFn,
//					 FPSEventCallbackFn			fpsFn,
//					 ChunkLODStatsCallbackFn	chunkFn,
//					 TextureListCallbackFn		chunkFn)
//{
//	remote::RemoteDebug::get_instance()->add_event<std::string>(stringFn);
//	remote::RemoteDebug::get_instance()->add_event<FrameTimeEvent>(fpsFn);
//	remote::RemoteDebug::get_instance()->add_event<ChunkLODStatsEvent>(chunkFn);
//	remote::RemoteDebug::get_instance()->add_event<TextureList>(chunkFn);
//	remote::RemoteDebug::get_instance()->add_event<TexturePixels>(chunkFn);
//}

template < class DataTy_ >
void register_event(typename remote::DataEvent<DataTy_>::ReceivedFn callback)
{
	remote::RemoteDebug::get_instance()->add_event<DataTy_>(callback);
}


//struct RemoteDebuggerInterface
//{
//	typedef std::function< void (std::string& msg) > StringEventCallbackFn;
//
//
//	//void set_text_message_event_callback(StringEventCallbackFn fn)
//	//{
//	//	_textMessageCallback = fn;
//	//}
//
//private:
//	//void text_message_event_callback(std::string& message)
//	//{
//	//	if(_textMessageCallback)
//	//		_textMessageCallback(message);
//	//}
//
//private:
//	remote::RemoteDebug _remote;
//	//StringEventCallbackFn _textMessageCallback;
//};

}

}

#endif // __EXPLORE2_REMOTE_DEBUG_INTERFACE_H__
