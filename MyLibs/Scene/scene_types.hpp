#if !defined(__SCENE_TYPES_HPP__)
#define __SCENE_TYPES_HPP__

namespace scene
{

namespace transform
{

struct NodeType
{
	enum type 
	{
		Transform,
		Group,
		Drawable,
		Light,
		Camera,
		MeshTransform
	};
};

}

}

#endif // __SCENE_TYPES_HPP__