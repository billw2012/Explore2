#include "commontypes.h"

namespace explore2 {;


double time_type_to_earth_days( time_type t )
{
	return (double)t / (1000.0 * 60.0 * 60.0 * 24.0);
}

double time_type_to_seconds( time_type t )
{
	return (double)t / 1000.0;
}

}