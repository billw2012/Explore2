#ifndef _SCENE_MATRIXTRAITS_HPP
#define _SCENE_MATRIXTRAITS_HPP

#include "Misc/hash.hpp"

namespace scene
{

namespace transform
{

	template < class FloatType, class HashType = hash::DefaultHashType >
	struct DefaultMatrixTraits
	{
		typedef FloatType value_type;
		typedef HashType hash_type;
	};
}

}

#endif // _SCENE_MATRIXTRAITS_HPP