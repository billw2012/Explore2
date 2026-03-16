#ifndef logging_h__
#define logging_h__

#include <fstream>
#include <sstream>

namespace vice {;

/*
 *	A logging system with strongly typed error level
 */

struct Logging
{
	Logging();

	enum ErrorLevel
	{
		Critical,
		Error,
		Warning,
		Message
	};

	struct LogMessage
	{
		friend struct Logging;

		LogMessage(const ErrorLevel& level, Logging* owner)
			: _owner(owner)
			, _level(level)
		{}

		~LogMessage() 
		{
			_owner->append(*this);
		}

		template < class Ty_ >
		LogMessage& operator << (const Ty_& val)
		{
			_ss << val;
			return *this;
		}

		LogMessage& operator << (std::ostream& (*pf)(std::ostream&));
		LogMessage& operator << (std::ios& (*pf)(std::ios&));
		LogMessage& operator << (std::ios_base& (*pf)(std::ios_base&));

	private:
		Logging* _owner;
		ErrorLevel _level;
		std::stringstream _ss;
	};

	LogMessage log (const ErrorLevel& errorLevel)
	{
		return LogMessage(errorLevel, this);
	}

	void append(const LogMessage& msg);

	static Logging& global();

private:
	std::ofstream _f;
};

}

#endif // logging_h__
