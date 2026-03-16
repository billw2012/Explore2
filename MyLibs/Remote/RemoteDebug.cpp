#include "RemoteDebug.h"

namespace remote {;

std::shared_ptr<RemoteDebug> RemoteDebug::_instance;

const unsigned short RemoteDebug::PORT = 50582;

using namespace boost::asio::ip;

RemoteDebug::RemoteDebug( Debugger_t, const address& gameIP /*= address::from_string("127.0.0.1")*/ ) 
	: _role(Debugger),
	_ioService(),
	_gameIP(gameIP, PORT)
{
	_clientSocket.reset(new tcp::socket(_ioService));
	_terminated.reset();
	_waitForEventsLoop.reset(new boost::thread(std::bind(&RemoteDebug::wait_for_events_loop, this)));
}

RemoteDebug::RemoteDebug( Game_t )
	: _role(Game),
	_ioService()
{
	try 
	{
		_acceptor.reset(new tcp::acceptor(_ioService, tcp::endpoint(tcp::v4(), PORT)));
		accept_new_socket();
	}
	catch(boost::system::system_error err)
	{

	}
}

RemoteDebug::~RemoteDebug()
{
	std::cout << "Shutting down RemoteDebug..." << std::endl;
	_acceptor.reset();
	_terminated.set();

	if(_waitForEventsLoop)
	{
		std::cout << "RemoteDebug: Waiting for read thread to terminate..." << std::endl;
		_waitForEventsLoop->join();
	}
	if(_acceptSocketThread)
	{
		std::cout << "RemoteDebug: Waiting for socket accept thread to terminate..." << std::endl;
		_acceptSocketThread->join();
	}

	std::cout << "RemoteDebug: closing sockets..." << std::endl;

	if(_clientSocket)
		_clientSocket->close();

	if(!_sockets.empty())
	{
		for(auto itr = _sockets.begin(); itr != _sockets.end(); ++itr)
		{
			if(*itr)
				(*itr)->close();
		}
	}
	if(!_readSocketThreads.empty())
	{
		std::cout << "RemoteDebug: Waiting for read threads to terminate..." << std::endl;
		for(auto itr = _readSocketThreads.begin(); itr != _readSocketThreads.end(); ++itr)
		{
			if(*itr)
			{
				(*itr)->join();
			}
		}
	}
	std::cout << "RemoteDebug shut down complete." << std::endl;
}

bool RemoteDebug::process()
{
	if(_role == Game)
		accept_waiting_sockets();

	boost::mutex::scoped_lock _lockQueue(_queueMutex);
	if(!_eventQueue.empty())
	{
		_eventQueue.front()->signal();
		_eventQueue.pop_front();
	}
	return !_eventQueue.empty();
}

RemoteDebug* RemoteDebug::get_instance()
{
	if(!_instance)
		return NULL;
	return _instance.get();
}

void RemoteDebug::create_instance( Game_t )
{
	_instance.reset(new RemoteDebug(Game_t()));
}

void RemoteDebug::create_instance( Debugger_t, const address& gameIP /*= address::from_string("127.0.0.1")*/ )
{
	_instance.reset(new RemoteDebug(Debugger_t(), gameIP));
}

void RemoteDebug::destroy()
{
	_instance.reset();
}

void RemoteDebug::accept_new_socket()
{
	_socketAccepted.reset();
	_newSocket.reset(new tcp::socket(_ioService));
	_acceptSocketThread.reset(new boost::thread(std::bind(&RemoteDebug::wait_for_socket, this)));
	//std::function<void(const boost::system::error_code& error)>
	//	newSockFn(std::bind(&RemoteDebug::new_socket_accepted, this, boost::asio::placeholders::error));
}

void RemoteDebug::accept_waiting_sockets()
{
	if(_socketAccepted.is_set())
	{
		_sockets.push_back(_newSocket);
		_readSocketThreads.push_back(std::shared_ptr<boost::thread>(new boost::thread(std::bind(&RemoteDebug::wait_for_events_server_loop, this, _newSocket))));
		accept_new_socket();
	}
}

void RemoteDebug::wait_for_socket()
{
	boost::system::error_code error;
	_acceptor->accept(*_newSocket, error);//, socket_connected);//std::bind(&RemoteDebug::new_socket_accepted, this, std::placeholders::_1));
	if(!error)
	{
		std::cout << "Debugger attached." << std::endl;
		_socketAccepted.set();
	}
}

void RemoteDebug::wait_for_events_server_loop(SocketPtr sock)
{
	while(sock->is_open() && !_terminated.is_set())
	{
		try
		{
			std::vector<unsigned char> tagBuffer(sizeof(int) + sizeof(size_t));
			boost::asio::read(*sock, boost::asio::buffer(tagBuffer, 8));

			//std::istrstream tagAndLen(&tagBuffer[0]);
			//boost::archive::binary_iarchive iaTagAndLen(tagAndLen);
			int messageTag = *((int*)&tagBuffer[0]);
			size_t strLen = *((size_t*)&tagBuffer[sizeof(int)]);
			//iaTagAndLen >> messageTag >> strLen;

			std::vector<char> data(strLen);
			boost::asio::read(*sock, boost::asio::buffer(data, strLen));

			std::istrstream str(&(data[0]), (int)strLen);
			boost::archive::binary_iarchive iaStr(str);
			Event::ptr newEvent(_events[messageTag-1]->clone());
			newEvent->read(iaStr);

			{
				boost::mutex::scoped_lock _lockQueue(_queueMutex);
				_eventQueue.push_back(newEvent);
			}
		}
		catch(boost::system::system_error&)
		{
			sock->close();
		}
	}
	std::cout << "Debugger disconnected." << std::endl;
}

void RemoteDebug::wait_for_events_loop()
{
	using namespace boost::asio::ip;

	//boost::asio::io_service localIoService;
	//tcp::socket sock(localIoService);
	while(!_terminated.is_set())
	{
		if(!_clientSocket->is_open())
		{
			try
			{
				_clientSocket->connect(_gameIP);
			}
			catch (boost::system::system_error&)
			{
				std::cout << "Failed to connect" << std::endl;
				_clientSocket->close();
				Sleep(20);
			}
		}
		if(_clientSocket->is_open())
		{
			try
			{
				std::vector<unsigned char> tagBuffer(sizeof(int) + sizeof(size_t));
				boost::asio::read(*_clientSocket, boost::asio::buffer(tagBuffer, 8));

				//std::istrstream tagAndLen(&tagBuffer[0]);
				//boost::archive::binary_iarchive iaTagAndLen(tagAndLen);
				int messageTag = *((int*)&tagBuffer[0]);
				size_t strLen = *((size_t*)&tagBuffer[sizeof(int)]);
				//iaTagAndLen >> messageTag >> strLen;

				std::vector<char> data(strLen);
				boost::asio::read(*_clientSocket, boost::asio::buffer(data, strLen));

				std::istrstream str(&(data[0]), (int)strLen);
				boost::archive::binary_iarchive iaStr(str);
				Event::ptr newEvent(_events[messageTag-1]->clone());
				newEvent->read(iaStr);

				{
					boost::mutex::scoped_lock _lockQueue(_queueMutex);
					_eventQueue.push_back(newEvent);
				}
			}
			catch(boost::system::system_error&)
			{
				_clientSocket->close();
			}
		}
	}
}


}