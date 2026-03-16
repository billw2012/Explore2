#ifndef timer_h__
#define timer_h__

namespace misc {;

struct Timer
{
	Timer()
	{
		::QueryPerformanceFrequency(&frequency);
		reset();
	}

	void reset()
	{
		::QueryPerformanceCounter(&lastVal);
	}

	double elapsed()
	{
		LARGE_INTEGER currVal;
		::QueryPerformanceCounter(&currVal);
		double elapsedTime = (double)((currVal.QuadPart - lastVal.QuadPart) * 1000.0 / frequency.QuadPart);
		lastVal = currVal;
		return elapsedTime;
	}

private:
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER lastVal;           // ticks
};

}

#endif // timer_h__
