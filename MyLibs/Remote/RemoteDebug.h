
#if !defined(__REMOTE_REMOTE_DEBUG_H__)
#define __REMOTE_REMOTE_DEBUG_H__

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <ctime>
#include <iostream>

#include <strstream>

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0501
#endif

#include <boost/asio.hpp>
#include <functional>
#include <boost/thread.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Misc/waithandle.hpp"

//#include "RemoteDLL.h"
//#include <boost/serialization/string.hpp>

namespace remote{;

struct Event
{
	typedef std::shared_ptr< Event > ptr;
	virtual void read(boost::archive::binary_iarchive& arch) = 0;
	virtual void signal() = 0;
	virtual Event* clone() const = 0;
};

template < class DataTy_ >
struct DataEvent : Event
{
	typedef DataTy_ data_type;
	typedef std::function<void (data_type& data)> ReceivedFn;
	

	DataEvent(ReceivedFn callback) : _receivedCallback(callback) {}

	virtual void read(boost::archive::binary_iarchive& arch)
	{
		arch >> _data;
	}

	virtual void signal()
	{
		_receivedCallback(_data);
	}

	data_type& get_data() { return _data; }

	static std::string s_id()
	{
		return typeid(DataTy_).name();
	}

	virtual Event* clone() const
	{
		return new DataEvent< data_type >(*this);
	}

private:
	data_type _data;
	ReceivedFn _receivedCallback;
};

struct /*REMOTE_API*/ RemoteDebug
{
	enum type {
		Game,
		Debugger
	};

	struct Game_t {};
	struct Debugger_t {};

	static const unsigned short PORT;

	// create as debugger (client)
	RemoteDebug(Debugger_t, const boost::asio::ip::address& gameIP =
		boost::asio::ip::make_address("127.0.0.1"));

	// create as game (server)
	RemoteDebug(Game_t);

	~RemoteDebug();

	template < class DataTy_ >
	void add_event(typename DataEvent<DataTy_>::ReceivedFn callback)
	{
		_events.push_back(Event::ptr(new DataEvent<DataTy_>(callback)));
		_handleTagMap[DataEvent<DataTy_>::s_id()] = (int)_events.size();
	}

	template < class DataTy_ >
	void send_event(DataTy_& data)
	{
		StringIntMap::const_iterator hItr = _handleTagMap.find(DataEvent<DataTy_>::s_id());
		assert(hItr != _handleTagMap.end());
		int tag = hItr->second;

		// serialize the data
		std::ostrstream str;
		boost::archive::binary_oarchive oaStr(str);
		oaStr << data; // serialize the tag and the data into a single stream
		//std::ostrstream tagAndLen;
		//boost::archive::binary_oarchive oaTagAndLen(tagAndLen);
		size_t count = str.pcount();
		std::vector<unsigned char> tagAndLen(sizeof(int) + sizeof(size_t));
		*((int*)&tagAndLen[0]) = tag;
		*((size_t*)&tagAndLen[sizeof(int)]) = count;
		//oaTagAndLen << tag << count;
		//tagAndLen.flush();
		//std::string tagAndLenStr = tagAndLen.str();

		//// check for any sockets that are pending connection
		//accept_waiting_sockets();

		if(_role == Game)
		{
			// write the stream to all connected sockets
			for(size_t idx = 0; idx < _sockets.size(); ++idx)
			{
				try
				{
					boost::system::error_code ignored_error;
					boost::asio::write(*(_sockets[idx]), boost::asio::buffer(tagAndLen, 8));
					boost::asio::write(*(_sockets[idx]), boost::asio::buffer(str.str(), str.pcount()));
				}
				catch(boost::system::system_error&)
				{
					// write failed so drop the connection
					_sockets[idx]->close();
					_sockets.erase(_sockets.begin() + idx);
					--idx;
				}
			}
		}
		else if(_clientSocket->is_open())
		{
			try
			{
				boost::system::error_code ignored_error;
				boost::asio::write(*_clientSocket, boost::asio::buffer(tagAndLen, 8));
				boost::asio::write(*_clientSocket, boost::asio::buffer(str.str(), str.pcount()));
			}
			catch(boost::system::system_error&)
			{
				// write failed so drop the connection
				_clientSocket->close();
			}
		}
	}

	bool process();

	static RemoteDebug* get_instance();

	static void create_instance(Game_t);

	static void create_instance(Debugger_t, const boost::asio::ip::address& gameIP =
		boost::asio::ip::make_address("127.0.0.1"));

	static void destroy();

private:
	void accept_new_socket();

	void accept_waiting_sockets();

	void wait_for_socket();

	typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
	void wait_for_events_server_loop(SocketPtr sock);

	void wait_for_events_loop();

private:
	static std::shared_ptr<RemoteDebug> _instance;
	
	boost::asio::io_context _ioService;
	SocketPtr _clientSocket;
	std::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor;
	boost::asio::ip::tcp::endpoint _gameIP;
	SocketPtr _newSocket;

	misc::WaitHandle _socketAccepted, _terminated;

	std::vector< SocketPtr > _sockets;

	type _role;

	std::shared_ptr<boost::thread> _waitForEventsLoop;

	std::vector< Event::ptr > _events;
	typedef std::map< std::string, int > StringIntMap;
	std::map< std::string, int > _handleTagMap;

	mutable boost::mutex _queueMutex;
	std::deque< Event::ptr > _eventQueue;

	std::shared_ptr<boost::thread> _acceptSocketThread;
	std::vector< std::shared_ptr<boost::thread> > _readSocketThreads;
};

}

#endif // __REMOTE_REMOTE_DEBUG_H__