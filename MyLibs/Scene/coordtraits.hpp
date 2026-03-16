#ifndef _SCENE_COORDTRAITS_HPP
#define _SCENE_COORDTRAITS_HPP

#include "Misc/hash.hpp"

namespace scene
{

namespace transform
{

	template < class FloatType, class HashType = hash::DefaultHashType >
	struct DefaultCoordinateTraits
	{
		typedef FloatType value_type;
		typedef HashType hash_type;
	};
}

}

#endif // _SCENE_COORDTRAITS_HPP