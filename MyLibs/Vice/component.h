#pragma once

// standard
#include <unordered_set>
#include <unordered_map>
#include <set>

// solution
#include "GLBase/texture.hpp"
#include "utils/time_type.h"

// project
#include "template.h"

namespace vice {;

/* 
 * Contains:
 *	Name of component class
 *	Animated scene
 *		Hierarchy of elements and component instances.
 *		Elements might be quads, lines, text boxes etc.
 *	References to any external resources needed
 *		Fonts
 *		Script files
 */

/*
 *	Component:
 *		All properties should be stored in a homogeneous format:
 *			id.property = value
 *			where id is the locally unique identifier of an element or component instance.
 *		Animations can be done by binding a set of <time, value> pairs to an id.property.
 *		
 */

struct ComponentInstance : public Object
{
	struct LayoutNode
	{
		typedef std::shared_ptr<LayoutNode> ptr;

		Object* object;

		LayoutNode(Object* object_ = nullptr) : object(object_) {}

		struct LayoutNodePtrSort
		{
			bool operator()(const LayoutNode::ptr& lhs, const LayoutNode::ptr& rhs) const
			{
				if (lhs->object->get_order() < rhs->object->get_order())
					return true;
				if (lhs->object->get_order() > rhs->object->get_order())
					return false;
				return lhs < rhs;
			}
		};

		std::set<LayoutNode::ptr, LayoutNodePtrSort> children;
	};

	typedef python_vice_ptr<ComponentInstance> ptr;

	friend struct ComponentLibrary;
	friend struct Object;

	ComponentInstance() : Object(std::string(), ComponentType) {}

	// get design width of component template
	float get_width_base() const;
	// get design height of component template
	float get_height_base() const;

	size_t get_child_count() const;
	Object::ptr::shared_ptr_type get_child(int idx) const;
	Object::ptr::shared_ptr_type get_child_by_name(const std::string& name) const;
	void add_child(const Object::ptr& obj);
	void add_child_python(const Object::ptr::shared_ptr_type& obj);
	void remove_child(const Object::ptr& obj);
	void remove_child_by_name(const std::string& name);

	void set_texture(const glbase::Texture::ptr& texture);
	const glbase::Texture::ptr& get_texture() const;

	void set_background(const glbase::Texture::ptr& background);
	const glbase::Texture::ptr& get_background() const;

	void update_animation();
	void update_layout();

	void update(const float t);

	// Functions overridden in derived python classes >>
	virtual void mouse_move(float x, float y) { std::cout << "mouse_move\n"; }
	virtual void mouse_down(float x, float y, int button) { std::cout << "mouse_down\n"; }
	virtual void mouse_up(float x, float y, int button) { std::cout << "mouse_up\n"; }
	virtual void update_python() { std::cout << "update_python\n"; }
	// << Functions overriden in derived python classes >>

#if defined(VICE_DESIGNER)

	// design mode functions >>>
	void set_root_transform(const math::Matrix4f& matrix) { _rootTransform = matrix; }
	void save(const boost::filesystem::path& file = boost::filesystem::path());
	ComponentTemplate::ptr get_template() const;
	const boost::filesystem::path& get_load_path() const { return _template->_fullPath; }
	std::string get_unique_name(const std::string& baseName) const;
	int get_top_order() const;
	bool contains_component(const boost::filesystem::path& path) const;
	Object::ptr get_child_by_ptr(const Object* obj) const;
	// synch properties of the component only to the template
	void synch_template_properties();
	// synch the entire template for this component
	void update_template();
	// << design mode functions

#endif

	const LayoutNode& get_layout_root() const { return _layoutRoot; }
	const LayoutNode* get_layout_node(const Object* obj) const;
	math::Matrix4f get_camera() const;

	Object* get_hit_object_global(float x, float y);
	Object* get_hit_object_global_ignore(float x, float y, const std::function<bool(const Object*)>& ignoreFn);

	static void register_type();

private:
	Object* recurse_hit(float x, float y, LayoutNode& node, bool root, const std::function<bool(const Object*)>& ignoreFn);

protected:
	void clear_animated_properties() override;
	void clear_layout_properties() override;

private:
	ComponentTemplate::ptr _template;
	// path this instance was created using, could be relative to an include directory or its owner, and
	// needs to be kept that way when saved.
	boost::filesystem::path _instancedAs;

	std::vector<Object::ptr> _children;

	glbase::Texture::ptr _texture, _background;

	LayoutNode _layoutRoot;
	std::unordered_map<const Object*, LayoutNode*> _objectLayoutMap;

	static void build_layout_heirarchy(const ComponentInstance* comp, LayoutNode& compNode, std::unordered_map<const Object*, LayoutNode*>& objLayoutMap);
	static void recursive_layout(LayoutNode* node, float layoutWidth, float layoutHeight);

	math::Matrix4f _rootTransform;
};

struct ComponentLibrary
{
	static void load_component(boost::filesystem::path name, const boost::filesystem::path& ownerPath = boost::filesystem::path());
	static ComponentInstance::ptr instance_component(const boost::filesystem::path& type, const std::string& name, const boost::filesystem::path& ownerPath = boost::filesystem::path());

	static void add_include_directory(const boost::filesystem::path& dir);
	static boost::filesystem::path resolve_path(boost::filesystem::path file, const boost::filesystem::path& context = boost::filesystem::path());

private:
	//static void apply_properties(const Object& obj, const ComponentTemplate::AnimatedPropertyMap& props );

	struct PathHash
	{
		size_t operator()(const boost::filesystem::path& path) const 
		{
			return boost::filesystem::hash_value(path);
			//std::string str = path.string();
			//std::transform(str.begin(), str.end(), str.begin(), ::toupper);
			//return std::hash<std::string>()(str);
		}
	};
	static std::unordered_map<boost::filesystem::path, ComponentTemplate::ptr, PathHash> _components;

	static std::unordered_set<boost::filesystem::path, PathHash> _includeDirectories;
};

}
