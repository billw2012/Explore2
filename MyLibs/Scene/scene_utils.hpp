#if !defined(__SCENE_UTILS_HPP__)
#define __SCENE_UTILS_HPP__

namespace scene
{

namespace transform
{

/*
To copy a hierarchy:
need to know type of each object and be able to create new ones
clone hierarchy function needs to return a specific type, this type can be known as it is the type given

*/

template < class NodeType >
typename NodeType::ptr copy_hierarchy(typename NodeType::ptr root)
{
	
}

template < class NodeType >
typename NodeType::ptr duplicate_node(typename NodeType::ptr node)
{
	switch(node->get_node_type())
	{
	case Transform:
		return 
	case Group:
	case Drawable:
	case MeshTransform:
	case Light:
	case Camera:
	};
}

}
}
#endif // __SCENE_UTILS_HPP__