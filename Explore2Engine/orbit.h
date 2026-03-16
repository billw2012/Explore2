
#ifndef orbit_h__
#define orbit_h__

#include "Math/vector2.hpp"
#include "Math/vector3.hpp"
#include "Math/matrix4.hpp"

namespace explore2 {;

struct Orbit
{
	//static final float SystemScale = 2000.0f;
	typedef double real_type;

	//real_type _localX, _localY;

	// Longitude means rotational offset from the primary axis of the system in orbital mechanics.
	// e.g. if rotation is around z axis then either x or y might be the primary axis from which longitudes are measured.
	// Perihelion is the point of closest approach to the orbited body.
	// longitudeOfPerihelion: the angle from primary axis of the point of closest approach.
	// meanDistance: average orbital distance.
	// dailyMotion: motion of orbiting body per Earth day.
	// eccentricity: how elliptical the orbit is.
	// meanLongitude: angle the orbit starts from ("Mean longitude is the ecliptic longitude at which an orbiting body could be found if its orbit were circular, and free of perturbations, and if its inclination were zero.")
	Orbit();
	Orbit(real_type longitudeOfPerihelion, real_type meanDistance, real_type dailyMotion, real_type eccentricity, real_type meanLongitude);

	//void update_local_position(float d);
	math::Vector2<real_type> get_position(double days, real_type meanLongitudeOffset = 0);
	math::Vector3<real_type> get_position(double days, const math::Matrix4<real_type>& rotation, real_type meanLongitudeOffset = 0);

private:
	real_type 
		_longitudeOfPerihelion, 
		_meanDistance, 
		_dailyMotion,
		_eccentricity, 
		_meanLongitude;
};

}

#endif // orbit_h__