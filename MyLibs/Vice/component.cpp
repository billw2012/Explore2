// standard
// boost
#include <boost/variant/multivisitors.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/container_hash/hash.hpp>

// external
#include "tinyxml.h"

// solution
#include "Utils/unique_vector.h"
#include "Math/transformation.hpp"

// project
#include "element.h"
#include "component.h"
#include "logging.h"
#include "script.h"
#include "xml_utils.h"

namespace vice {;

float ComponentInstance::get_width_base() const
{
	return get_property_or_default(_template->_properties, "width", 0.0f);
}

float ComponentInstance::get_height_base() const
{
	return get_property_or_default(_template->_properties, "height", 0.0f);
}

size_t ComponentInstance::get_child_count() const
{
	return _children.size();
}

Object::ptr::shared_ptr_type ComponentInstance::get_child(int idx) const
{
	return _children[idx].ptr();
}

Object::ptr::shared_ptr_type ComponentInstance::get_child_by_name( const std::string& name ) const
{
	auto itr = boost::find_if(_children, [&name](const Object::ptr& obj) -> bool { return obj->get_name() == name; });
	if(itr != std::end(_children))
		return itr->ptr();
	return Object::ptr::shared_ptr_type();
}

void ComponentInstance::add_child(const Object::ptr& obj)
{
	obj->set_owner(this);
}

void ComponentInstance::add_child_python(const Object::ptr::shared_ptr_type& obj)
{
	add_child(Object::ptr(obj));
}

void ComponentInstance::remove_child(const Object::ptr& obj)
{
	if(boost::find(_children, obj) != std::end(_children))
		obj->set_owner(nullptr);
}

void ComponentInstance::remove_child_by_name(const std::string& name)
{
	remove_child(get_child_by_name(name));
}

void ComponentInstance::set_texture(const glbase::Texture::ptr& texture)
{
	_texture = texture;
}

const glbase::Texture::ptr& ComponentInstance::get_texture() const
{
	return _texture;
}

void ComponentInstance::set_background(const glbase::Texture::ptr& background)
{
	_background = background;
}

const glbase::Texture::ptr& ComponentInstance::get_background() const
{
	return _background;
}

void ComponentInstance::update(const float t)
{
	/*
	recursively for each component
		apply animation
		perform layout based on animated values
		
	*/
	clear_animated_properties();
	clear_layout_properties();
	//boost::for_each(_children, [](Object::ptr& obj) {
	//	obj->clear_animated_properties();
	//	obj->clear_layout_properties();
	//});
	update_animation();
	update_layout();
}

void ComponentInstance::clear_animated_properties()
{
	Object::clear_animated_properties();
	boost::for_each(_children, [](Object::ptr& obj) {
		obj->clear_layout_properties();
	});
}

void ComponentInstance::clear_layout_properties()
{
	Object::clear_layout_properties();
	boost::for_each(_children, [](Object::ptr& obj) {
		obj->clear_layout_properties();
	});
}

void ComponentInstance::update_animation()
{
}

float layout_dim(float dim, float parentBaseDim, float parentLayoutDim, Object::PinType::type pin)
{
	switch (pin)
	{
	case Object::PinType::PinMin:
		// pin left, or top so value doesn't change
		return dim;
	case Object::PinType::PinMax:
		// pin right, or bottom so value stays fixed with respect to right or top
		return std::max<float>(0, dim + (parentLayoutDim - parentBaseDim));
	case Object::PinType::Stretch:
	default:
		// stretch, so linearly scale value by same factor as parent changed
		return dim * (parentLayoutDim / parentBaseDim);
	};
}

void ComponentInstance::recursive_layout(LayoutNode* node, float layoutWidth, float layoutHeight)
{
	float baseWidth;
	float baseHeight;
	if (node->object->get_type() == Object::ComponentType)
	{
		auto comp = static_cast<ComponentInstance*>(node->object);
		baseWidth = comp->get_width_base();
		baseHeight = comp->get_height_base();
	}
	else
	{
		baseWidth = node->object->get_width();
		baseHeight = node->object->get_height();
	}
	float widthScale = layoutWidth / baseWidth;
	float heightScale = layoutHeight / baseHeight;
	node->object->set_width_layout(layoutWidth);
	node->object->set_height_layout(layoutHeight);

	for (auto itr = node->children.begin(); itr != node->children.end(); ++itr)
	{
		auto child = *itr;
		auto childObj = child->object;
		float childX = layout_dim(childObj->get_x(), baseWidth, layoutWidth, childObj->get_layout_left());
		float childY = layout_dim(childObj->get_y(), baseHeight, layoutHeight, childObj->get_layout_top());
		float newWidth = layout_dim(childObj->get_x() + childObj->get_width(), baseWidth, layoutWidth, childObj->get_layout_right()) - childX;
		float newHeight = layout_dim(childObj->get_y() + childObj->get_height(), baseHeight, layoutHeight, childObj->get_layout_bottom()) - childY;
		childObj->set_x_layout(childX);
		childObj->set_y_layout(childY);

		childObj->transform = math::translate<float>(childObj->get_x(), childObj->get_y(), 0.0f);
		childObj->globalTransform = node->object->globalTransform * childObj->transform;

		if (node->object->get_type() == Object::ComponentType)
			childObj->componentTransform = childObj->transform;
		else
			childObj->componentTransform = node->object->componentTransform * childObj->transform;

		recursive_layout(child.get(), newWidth, newHeight);
	}
}

void ComponentInstance::update_layout()
{
	assert(_texture);

	_objectLayoutMap.clear();
	_layoutRoot = LayoutNode(this);
	_objectLayoutMap[this] = &_layoutRoot;
	build_layout_heirarchy(this, _layoutRoot, _objectLayoutMap);
	this->componentTransform = math::Matrix4f{};
	this->transform = this->globalTransform = this->_rootTransform;

#if defined(VICE_DESIGNER)
	recursive_layout(&_layoutRoot, get_width_base(), get_height_base());
#else
	recursive_layout(&_layoutRoot, static_cast<float>(_texture->width()), static_cast<float>(_texture->height()));
#endif

	//// layout child components
	//for (auto itr = _children.begin(); itr != _children.end(); ++itr)
	//{
	//	auto child = *itr;
	//	if (child->get_type() == Object::ObjectType::ComponentType)
	//	{
	//		auto component = static_cast<ComponentInstance*>(child.get());
	//		component->update_layout();
	//	}
	//}
}

void ComponentInstance::build_layout_heirarchy(const ComponentInstance* comp, LayoutNode& compNode, std::unordered_map<const Object*, LayoutNode*>& objLayoutMap)
{
	std::unordered_map<const Object*, LayoutNode::ptr> nodeMap;


	for (auto itr = comp->_children.begin(); itr != comp->_children.end(); ++itr)
	{
		auto child = *itr;
		LayoutNode::ptr node = std::make_shared<LayoutNode>(child.get());
		if (child->get_type() == Object::ComponentType)
		{
			build_layout_heirarchy(static_cast<ComponentInstance*>(child.get()), *node, objLayoutMap);
		}
		nodeMap[child.get()] = node;
		objLayoutMap[child.get()] = node.get();
	}

	for (auto itr = nodeMap.begin(); itr != nodeMap.end(); ++itr)
	{
		std::string parent = itr->first->get_parent();
		if (parent.empty())
			compNode.children.insert(itr->second);
		else
		{
			auto parentObj = comp->get_child_by_name(parent);
			if (parentObj)
				nodeMap[parentObj.get()]->children.insert(itr->second);
		}
	}
}

#if defined(VICE_DESIGNER)

void ComponentInstance::synch_template_properties()
{
	_template->_properties = _properties;
}

void ComponentInstance::update_template()
{
	_template->_properties = _properties;
	_template->_components.clear();
	_template->_elements.clear();
	
	for (const auto& obj : _children)
	{
		switch (obj->get_type())
		{
		case Object::ElementType:
		{
			auto& templateElem = _template->_elements[obj->get_name()];
			templateElem.properties = obj->get_properties();
			break;
		};
		case Object::ComponentType:
		{
			auto& templateComp = _template->_components[obj->get_name()];
			templateComp.properties = obj->get_properties();
			auto temp = static_cast<ComponentInstance*>(obj.get());
			templateComp.path = temp->_instancedAs;
			break;
		};
		}
	}
}

template < class Cont_ >
void save_properties(const Cont_& propMap, TiXmlElement* parentElement)
{
	for (const auto& prop : propMap)
	{
		TiXmlElement* propNode = new TiXmlElement("property");
		propNode->SetAttribute("name", prop.first.c_str());
		auto valTypePair = Object::property_to_string(prop.second);
		propNode->SetAttribute("value", valTypePair.val.c_str());
		propNode->SetAttribute("type", Object::PropType::to_string(valTypePair.type).c_str());
		parentElement->LinkEndChild(propNode);
	}
}

void ComponentInstance::save(const boost::filesystem::path& file /*= boost::filesystem::path()*/)
{
	update_template();

	TiXmlDocument doc;

	TiXmlElement* root = new TiXmlElement("component");

	root->SetAttribute("name", _template->_name.c_str());
	
	save_properties(_template->_properties, root);

	for (auto elem : _template->_elements)
	{
		TiXmlElement* elemNode = new TiXmlElement("element");
		elemNode->SetAttribute("name", elem.first.c_str());
		save_properties(elem.second.properties, elemNode);
		root->LinkEndChild(elemNode);
	}

	for (auto comp : _template->_components)
	{
		TiXmlElement* compNode = new TiXmlElement("component");
		compNode->SetAttribute("name", comp.first.c_str());
		compNode->SetAttribute("path", comp.second.path.string().c_str());
		save_properties(comp.second.properties, compNode);
		root->LinkEndChild(compNode);
	}

	for (auto script : _template->_scripts)
	{
		TiXmlElement* node = new TiXmlElement("script");
		node->SetAttribute("path", script.string().c_str());
		root->LinkEndChild(node);
	}

	doc.LinkEndChild(root);

	if (file.empty())
		doc.SaveFile(_template->_fullPath.string().c_str());
	else
		doc.SaveFile(file.string().c_str());
}

ComponentTemplate::ptr ComponentInstance::get_template() const
{
	return _template;
}

std::string ComponentInstance::get_unique_name(const std::string& baseName) const
{
	int idx = 1;
	std::stringstream ss;
	ss << baseName << idx;
	while (boost::range::find_if(_children, [&](const Object::ptr& obj)->bool {
		return obj->get_name() != ss.str();
	}) == _children.end())
	{
		++idx;
		ss = std::stringstream();
		ss << baseName << idx;
	}
	return ss.str();
}

int ComponentInstance::get_top_order() const
{
	int idx = 0;
	for (auto child : _children)
	{
		idx = std::max<int>(idx, child->get_order());
	}
	return idx;
}

bool ComponentInstance::contains_component(const boost::filesystem::path& path) const
{
	if (get_load_path() == path)
		return true;
	for (auto child : _children)
	{
		if (child->get_type() == Object::ComponentType)
		{
			auto comp = static_cast<ComponentInstance*>(child.get());
			if (comp->contains_component(path))
				return true;
		}
	}
	return false;
}

Object::ptr ComponentInstance::get_child_by_ptr(const Object* obj) const
{
	auto fItr = boost::range::find_if(_children, [obj](const Object::ptr& child) -> bool {
		return child.get() == obj;
	});
	if (fItr != _children.end())
		return *fItr;
	return Object::ptr();
}

#endif

math::Matrix4f ComponentInstance::get_camera() const
{
#if defined(VICE_DESIGNER)
	return math::ortho<float>(0.0f, (float)_texture->width(), (float)_texture->height(), 0.0f, -1.0f, 1.0f);
#else
	return math::ortho<float>(0.0f, (float)get_width(), (float)get_height(), 0.0f, -1.0f, 1.0f);
#endif		
}

const ComponentInstance::LayoutNode* ComponentInstance::get_layout_node(const Object* obj) const
{
	auto fItr = _objectLayoutMap.find(obj);
	if (fItr == _objectLayoutMap.end())
		return nullptr;
	return fItr->second;
}

Object* ComponentInstance::recurse_hit(float x, float y, LayoutNode& node, bool root, const std::function<bool(const Object*)>& ignoreFn)
{
	if (ignoreFn(node.object))
		return nullptr;

	if (node.object->is_hit_global(x, y))
	{
		if (!root && node.object->get_type() == Object::ComponentType)
			return node.object;

		for (auto& childNode : node.children)
		{
			Object* hit = recurse_hit(x, y, *childNode, false, ignoreFn);
			if (hit)
				return hit;
		}
		return node.object;
	}
	return nullptr;
}

Object* ComponentInstance::get_hit_object_global(float x, float y)
{
	return recurse_hit(x, y, _layoutRoot, true, [](const Object*) -> bool { return false; });
}

Object* ComponentInstance::get_hit_object_global_ignore(float x, float y, const std::function<bool(const Object*)>& ignoreFn)
{
	return recurse_hit(x, y, _layoutRoot, true, ignoreFn);
}

// Wrapper to handle virtual functions overridden in derived classes in Python
struct ComponentInstanceVirtualWrapper : public ComponentInstance, public boost::python::wrapper<ComponentInstance> 
{
	void mouse_move(float x, float y) 
	{
		if (boost::python::override f = this->get_override("mouse_move")) 
			f(x, y);
		else 
			ComponentInstance::mouse_move(x, y);
	}
	void default_mouse_move(float x, float y) 
	{ 
		return ComponentInstance::mouse_move(x, y); 
	}

	void mouse_down(float x, float y, int button) 
	{
		if (boost::python::override f = this->get_override("mouse_down")) 
			f(x, y, button);
		else 
			ComponentInstance::mouse_down(x, y, button);
	}
	void default_mouse_down(float x, float y, int button)
	{ 
		return ComponentInstance::mouse_down(x, y, button); 
	}

	void mouse_up(float x, float y, int button)
	{
		if (boost::python::override f = this->get_override("mouse_up"))
			f(x, y, button);
		else
			ComponentInstance::mouse_up(x, y, button);
	}
	void default_mouse_up(float x, float y, int button)
	{
		return ComponentInstance::mouse_up(x, y, button);
	}

	void update_python()
	{
		if (boost::python::override f = this->get_override("update"))
			f();
		else
			ComponentInstance::update_python();
	}
	void default_update_python()
	{
		return ComponentInstance::update_python();
	}
};

void ComponentInstance::register_type()
{
	using namespace boost::python;
	class_<
		ComponentInstanceVirtualWrapper, 
		boost::noncopyable,
		bases<Object>,
		std::shared_ptr<ComponentInstanceVirtualWrapper>
	>
		("Component")
		.add_property("child_count", &ComponentInstance::get_child_count, "Number of child objects")
		.def("add_child", &ComponentInstance::add_child_python, "Get child by name")
		.def("get_child_by_name", &ComponentInstance::get_child_by_name, "Get child by name")
		.def("get_child", &ComponentInstance::get_child, "Get child by name")
		.def("mouse_move", &ComponentInstanceVirtualWrapper::mouse_move, &ComponentInstanceVirtualWrapper::default_mouse_move, "Override this to handle mouse movement within your component")\
		.def("mouse_down", &ComponentInstanceVirtualWrapper::mouse_down, &ComponentInstanceVirtualWrapper::default_mouse_down,  "Override this to handle mouse button down within your component")\
		.def("mouse_up", &ComponentInstanceVirtualWrapper::mouse_up, &ComponentInstanceVirtualWrapper::default_mouse_up, "Override this to handle mouse button up within your component")\
		.def("update", &ComponentInstanceVirtualWrapper::update_python, &ComponentInstanceVirtualWrapper::default_update_python, "Override this to perform per frame updates to your component")\
		;

	//implicitly_convertible<std::shared_ptr<ComponentInstance>, std::shared_ptr<Object> >();
}

void ComponentLibrary::load_component(boost::filesystem::path name, const boost::filesystem::path& ownerPath /*= boost::filesystem::path()*/)
{
	name = ComponentLibrary::resolve_path(name, ownerPath);
	if(_components.find(name) != _components.end())
		return ;

	ComponentTemplate::ptr comp = std::make_shared<ComponentTemplate>();

	comp->load(name);

	_components[name] = comp;

	// load sub components
	for(auto itr = comp->_components.begin(); itr != comp->_components.end(); ++itr)
		load_component(itr->second.path, name.parent_path());
}


template < class Cont_ >
void insert_and_replace(Cont_& target, Cont_& src)
{
	boost::for_each(src, [&target](const Cont_::value_type& keyVal) {
		target[keyVal.first] = keyVal.second;
	});
}

ComponentInstance::ptr ComponentLibrary::instance_component(const boost::filesystem::path& type, const std::string& name, const boost::filesystem::path& ownerPath /*= boost::filesystem::path()*/)
{
	using namespace boost::python;

	auto resolvedType = resolve_path(type, ownerPath);
	auto compItr = _components.find(resolvedType);
	if(compItr == _components.end())
	{
		return ComponentInstance::ptr();
	}
	ComponentTemplate::ptr comp = compItr->second;

	// execute the scripts
	for(auto itr = comp->_scripts.begin(); itr != comp->_scripts.end(); ++itr)
	{
		ScriptLibrary::execute_script(*itr, resolvedType.parent_path());
	}

	// create an instance
	std::string componentTypeName = comp->_name; //type.stem().string();

	object compModule = import(componentTypeName.c_str());
	object compNamespace = compModule.attr("__dict__");
	object mainModule = import("__main__");
	object mainMamespace = mainModule.attr("__dict__");

	std::stringstream cmd;
	cmd << componentTypeName << "()";

	object obj = eval(cmd.str().c_str(), mainMamespace, compNamespace);
	
	std::shared_ptr<ComponentInstanceVirtualWrapper> pInst = extract< std::shared_ptr<ComponentInstanceVirtualWrapper> >(obj);
	ComponentInstance::ptr inst(pInst, obj);

	inst->_name = name;
	inst->_template = comp;
	inst->_instancedAs = type;
	inst->_properties = comp->_properties;

	// create child instances
	for(auto itr = comp->_components.begin(); itr != comp->_components.end(); ++itr)
	{
		ComponentInstance::ptr child = instance_component(itr->second.path, itr->first, resolvedType.parent_path());
		// overwrite properties
		insert_and_replace(child->_properties, itr->second.properties);
		inst->add_child(child);
	}

	// elements
	for(auto itr = comp->_elements.begin(); itr != comp->_elements.end(); ++itr)
	{
		Element::ptr element(new Element(itr->first));
		insert_and_replace(element->_properties, itr->second.properties);
		inst->add_child(element);
	}

	return inst;
}


void ComponentLibrary::add_include_directory(const boost::filesystem::path& dir)
{
	_includeDirectories.insert(dir);
}

boost::filesystem::path ComponentLibrary::resolve_path(boost::filesystem::path file, const boost::filesystem::path& context /*= boost::filesystem::path()*/)
{
	using namespace boost::filesystem;

	boost::filesystem::path finalPath;
	if (file.is_absolute())
		finalPath = file;
	else if (exists(current_path() / file))
		finalPath = current_path() / file;
	else if (!context.empty() && exists(context / file))
		finalPath = context / file;
	else
	{
		auto itr = boost::find_if(_includeDirectories, [&file](const path& dir) -> bool {
			return exists(dir / file);
		});

		if (itr != _includeDirectories.end())
			finalPath = *itr / file;
	}
	finalPath = canonical(finalPath);
	finalPath.make_preferred();
	return finalPath;
}

//void ComponentLibrary::apply_properties( const Object& obj, const ComponentTemplate::AnimatedPropertyMap& props )
//{
//	obj._properties
//}

std::unordered_map<boost::filesystem::path, ComponentTemplate::ptr, ComponentLibrary::PathHash> ComponentLibrary::_components;
std::unordered_set<boost::filesystem::path, ComponentLibrary::PathHash> ComponentLibrary::_includeDirectories;

}
