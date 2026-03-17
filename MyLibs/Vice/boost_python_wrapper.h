#pragma once
#pragma warning(push)
#pragma warning(disable: 4244 4273 4244)
#define HAVE_ROUND
#define HAVE_HYPOT
#include <boost/python.hpp>
#pragma warning(pop)
// Python 2→3 compat: BOOST_PYTHON_MODULE(Vice) now generates PyInit_Vice, not initVice
#if PY_MAJOR_VERSION >= 3
#define initVice PyInit_Vice
#endif
