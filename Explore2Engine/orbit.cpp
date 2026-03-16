
#include "orbit.h"
#include "Math/constants.hpp"
#include "Math/vector4.hpp"

#include <cmath>


namespace explore2 {;

Orbit::Orbit() :
	_longitudeOfPerihelion(0),
	_meanDistance(0),
	_dailyMotion(0),
	_eccentricity(0),
	_meanLongitude(0)
{

}

Orbit::Orbit( real_type longitudeOfPerihelion, real_type meanDistance, real_type dailyMotion, real_type eccentricity, real_type meanLongitude ) 
	: _longitudeOfPerihelion(longitudeOfPerihelion),
	_meanDistance(meanDistance),
	_dailyMotion(dailyMotion), 
	_eccentricity(eccentricity), 
	_meanLongitude(meanLongitude)
{

}

Orbit::real_type mod_2pi( Orbit::real_type val )
{
	while(val > math::constants::pi*2) 
		val -= static_cast<Orbit::real_type>(math::constants::pi)*2;
	while(val < 0)
		val += static_cast<Orbit::real_type>(math::constants::pi)*2;
	return val;
}

math::Vector2<Orbit::real_type> Orbit::get_position( double days, real_type meanLongitudeOffset /*= 0*/ )
{
	real_type meanAnomaly = mod_2pi(_dailyMotion * static_cast<real_type>(days) + _meanLongitude + meanLongitudeOffset - _longitudeOfPerihelion);
	real_type trueAnomaly = meanAnomaly + 1 * ((2 * _eccentricity - std::pow(_eccentricity, 3) / 4) * std::sin(meanAnomaly)
		+ 5/4 * std::pow(_eccentricity, 2) * std::sin(2*meanAnomaly) + 13/12 * std::pow(_eccentricity, 3) * std::sin(3*meanAnomaly));
	real_type radiusVector  = _meanDistance * (1 - std::pow(_eccentricity, 2)) / (1 + _eccentricity * std::cos(trueAnomaly));
	math::Vector2<real_type> pos;
	pos.x = (float) (radiusVector * std::cos(trueAnomaly + _longitudeOfPerihelion));
	pos.y = (float) (radiusVector * std::sin(trueAnomaly + _longitudeOfPerihelion));
	return pos;
}

math::Vector3<Orbit::real_type> Orbit::get_position( double days, const math::Matrix4<real_type>& rotation, real_type meanLongitudeOffset /*= 0*/ )
{
	math::Vector2<real_type> planePos = get_position(days, meanLongitudeOffset);
	return math::Vector3<real_type>(rotation * math::Vector4<real_type>(planePos, static_cast<real_type>(0), static_cast<real_type>(1)));
}


}