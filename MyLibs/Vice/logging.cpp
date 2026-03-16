
#include "logging.h"

namespace vice {;

Logging::Logging()
	: _f("logs/vice.log")
{

}

Logging& Logging::global()
{
	static Logging globalLog;
	return globalLog;
}

void Logging::append( const LogMessage& msg )
{
	_f << msg._ss.str() << "\n";
}

Logging::LogMessage& Logging::LogMessage::operator << ( std::ostream& (*pf)(std::ostream&) )
{
	_ss << pf;
	return *this;
}

Logging::LogMessage& Logging::LogMessage::operator << ( std::ios& (*pf)(std::ios&) )
{
	_ss << pf;
	return *this;
}

Logging::LogMessage& Logging::LogMessage::operator << ( std::ios_base& (*pf)(std::ios_base&) )
{
	_ss << pf;
	return *this;
}

}

