#pragma once

#include <chrono>

namespace utils {;

using clock_type = std::chrono::high_resolution_clock;
using time_type = clock_type::time_point;
using seconds = std::chrono::duration<float>;

}