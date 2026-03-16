#ifndef _SCENE_GROUP_HPP
#define _SCENE_GROUP_HPP

#include <set>
#include <functional>
#include <cassert>
#include <algorithm>

//#define BOOST_BIND_ENABLE_STDCALL

#include <string>
#include "transform.hpp"
#include "matrixtraits.hpp"

namespace scene {;

namespace transform {;

struct Group : public Transform
{
	typedef std::shared_ptr< Group > ptr;
	//typedef Transform< MatrixFloatType, MatrixTraits >  base_type;
public:

	typedef Transform ChildType;
	typedef std::set< ChildType::ptr > ChildPtrList;
	typedef ChildPtrList::size_type size_type;
	typedef ChildPtrList::iterator ChildIterator;
	typedef ChildPtrList::const_iterator ConstChildIterator;

private:
	ChildPtrList _children;

public:

	Group(const std::string& name = "unnamedGroup") : Transform(name), _children()  {}

	void clone_child(ChildType::ptr child, Group* parent) const 
	{
		Transform::ptr newChild(child->clone());
		ChildType::ptr typedChild = std::dynamic_pointer_cast<ChildType>(newChild);
		assert(typedChild != NULL);
		parent->insert(typedChild);
	}

	virtual void clone_data(Transform* to) const
	{
		Group* typedTo = dynamic_cast<Group*>(to);
		assert(typedTo != NULL);
		typedTo->remove_all();
		Transform::clone_data(typedTo);
		std::for_each(begin(), end(), std::bind(&Group::clone_child, this, std::placeholders::_1, typedTo));
	}

	virtual Transform* clone() const
	{
		Group* newGroup = new Group(*this);
		clone_data(newGroup);
		return newGroup;
	}

	size_type size() const
	{
		return _children.size();
	}

	void insert( ChildType::ptr child )
	{
		_children.insert(child);
		child->set_parent(this);
	}

	void remove( ChildType::ptr child )
	{
		_children.erase(child);
		child->set_parent(NULL);
	}

	void remove_all()
	{
		struct UnparentFn
		{
			void operator()(ChildType::ptr child)
			{
				child->set_parent(NULL);
			}
		};
		std::for_each(begin(), end(), UnparentFn());
		_children.clear();
	}

	ChildIterator begin()
	{
		return _children.begin();
	}

	ConstChildIterator begin() const
	{
		return _children.begin();
	}

	ChildIterator end()
	{
		return _children.end();
	}

	ConstChildIterator end() const
	{
		return _children.end();
	}

	virtual void dirtyGlobalTransform() 
	{
		Transform::dirtyGlobalTransform();
		std::for_each(_children.begin(), _children.end(), [&](ChildType::ptr child) { child->dirtyGlobalTransform(); });
	}

	virtual NodeType::type get_node_type() const
	{
		return NodeType::Group;
	}

	struct MatchName
	{
		MatchName(const std::string& name) : _name(name) {}
		
		bool operator()(Transform* trans) const
		{
			return trans->get_name() == _name;
		}

	private:
		std::string _name;
	};

	template < class PredicateFn >
	void find_all(std::vector<Transform*>& found, PredicateFn pred) const
	{
		for(ConstChildIterator cItr = begin(); cItr != end(); ++cItr)
		{
			if(pred(*cItr))
				found.push_back(*cItr);
			if(Group* grp = dynamic_cast<Group*>(*cItr))
				grp->find_all(found, pred);
		}
	}

	template < class PredicateFn >
	Transform* find_first(PredicateFn pred) const
	{
		for(ConstChildIterator cItr = begin(); cItr != end(); ++cItr)
		{
			if(pred(*cItr))
				return *cItr;
			if(Group* grp = dynamic_cast<Group*>(*cItr))
				if(Transform* found = grp->find_first(pred))
					return found;
		}
		return NULL;
	}

	virtual AnimatedTransform::TimeRange get_anim_range() const
	{
		AnimatedTransform::TimeRange range = Transform::get_anim_range();
		for(ConstChildIterator cItr = begin(); cItr != end(); ++cItr)
			range.extend((*cItr)->get_anim_range());
		return range;
	}

	virtual void set_t(float t)
	{
		Transform::set_t(t);
		std::for_each(begin(), end(), std::bind(&ChildType::set_t, std::placeholders::_1, t));
	}
};

}

}

#endif // _SCENE_GROUP_HPP