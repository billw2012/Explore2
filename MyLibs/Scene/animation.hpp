#if !defined(__SCENE_ANIMATION_HPP__)
#define __SCENE_ANIMATION_HPP__



#include <map>
#include <memory>

#include "math/quaternion.hpp"
#include "math/vector3.hpp"
#include "math/matrix4.hpp"
#include "math/transformation.hpp"

#undef min
#undef max

namespace scene
{

struct Interpolation { enum type {
	None,
	Linear
};};

struct AnimatedTransform
{
	typedef std::shared_ptr < AnimatedTransform > ptr;

	struct TimeRange
	{
		TimeRange(float start = 0.0f, float end = 0.0f) : _start(start), _end(end) {}

		float get_start() const { return _start; }
		float get_end() const { return _end; }

		void extend(float t)
		{
			_start = std::min(t, _start);
			_end = std::max(t, _end);
		}

		void extend(TimeRange range)
		{
			_start = std::min(_start, range.get_start());
			_end = std::max(_end, range.get_end());
		}

	private:
		float _start, _end;
	};

	struct Key
	{
		Key() : _t(0.0f), _interp(Interpolation::Linear) {}

		Key(float t, const math::Quaternionf& rotate, 
			const math::Vector3f& translate, 
			const math::Vector3f& scale, 
			Interpolation::type interp = Interpolation::Linear) 
			: _t(t), _translate(translate), _rotate(rotate), 
			_scale(scale), _interp(interp) {}

		float get_t() const { return _t; }

		const math::Vector3f& get_translate() const { return _translate; }
		const math::Quaternionf& get_rotate() const { return _rotate; }
		const math::Vector3f& get_scale() const { return _scale; }
		
		Interpolation::type get_interp() const { return _interp; }

	private:
		float _t;
		math::Vector3f _translate, _scale;
		math::Quaternionf _rotate;
		Interpolation::type _interp;
	};

	void add_key(const Key& key)
	{
		_keys[key.get_t()] = key;
	}

	math::Matrix4d get_trans(float t)
	{
		Key key = get_key(t);

		return math::Matrix4d(math::transform(key.get_rotate(), key.get_translate(), key.get_scale()));
	}

	TimeRange get_range() const
	{
		if(_keys.size() == 0)
			return TimeRange();
		if(_keys.size() == 1)
			return TimeRange(_keys.begin()->second.get_t(), _keys.begin()->second.get_t());
		return TimeRange(_keys.begin()->second.get_t(), (--_keys.end())->second.get_t());
	}

private:

	Key get_key(float t) const
	{
		if(_keys.size() == 0)
			return Key();
		if(_keys.size() == 1)
			return _keys.begin()->second;

		KeyMap::const_iterator kItr = _keys.lower_bound(t);
		if(kItr == _keys.begin())
			return _keys.begin()->second;
		if(kItr == _keys.end())
			return (--_keys.end())->second;
		if(kItr->first == t)
			return kItr->second;
		KeyMap::const_iterator prevK = kItr;
		--prevK;
		const Key& startKey = prevK->second;
		const Key& endKey = kItr->second;
		//if(startKey.get_interp() == Interpolation::Linear)
		float e = (t - startKey.get_t()) / (endKey.get_t() - startKey.get_t());
		return interpolate_keys(startKey, endKey, e);
	}

	template < class Ty_ > 
	static Ty_ interp(Ty_ a, Ty_ b, float e)
	{
		return (b - a) * e + a;
	}

	static Key interpolate_keys(const Key& startKey, const Key& endKey, float e)
	{
		return Key(interp(startKey.get_t(), endKey.get_t(), e),
			startKey.get_rotate().slerp(endKey.get_rotate(), e),
			interp(startKey.get_translate(), endKey.get_translate(), e),
			interp(startKey.get_scale(), endKey.get_scale(), e));
	}

private:
	typedef std::map< float, Key > KeyMap;
	std::map< float, Key > _keys;
};

//struct Animation
//{
//	typedef std::shared_ptr < Animation > ptr;
//
//	typedef std::map< std::string, AnimatedTransform::ptr > AnimatedTransformMap;
//
//	void add_animated_transform(const std::string& name, AnimatedTransform::ptr animTrans)
//	{
//		_animatedTransforms.insert(AnimatedTransformMap::value_type(name, animTrans));
//	}
//
//	void update_animation(Drawable::ptr root, )
//private:
//	AnimatedTransformMap _animatedTransforms;
//};

}

#endif // __SCENE_ANIMATION_HPP__