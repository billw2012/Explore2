#ifndef _SCENE_MATERIAL_H
#define _SCENE_MATERIAL_H

#include <string>
#include <map>
#include <unordered_map>
#include <set>

#include "Effect/effect.h"


namespace scene {;

struct Material
{
	typedef std::shared_ptr< Material > ptr;

private:
	typedef std::unordered_map< std::string, effect::Effect::ParameterVariantType > ParameterMap;

public:
	~Material() 
	{
		if(_boundMaterial == this)
			_boundMaterial = NULL;
	}

	// TYPE IS IMPORTANT: setting an int is NOT THE SAME as setting a float
	void set_parameter(const std::string& name, 
		const effect::Effect::ParameterVariantType& value)
	{
		_params[name] = value;
		//if(_boundMaterial == this)
		//	_effect->set_parameter(name, value, _boundMode);
	}

	void set_effect(effect::Effect::ptr effect)
	{
		_effect = effect;
	}

	effect::Effect::ptr get_effect() const { return _effect; }

	void bind(effect::Effect::EffectMode::type mode = effect::Effect::EffectMode::Render) const
	{
		if(_boundMaterial != this)
		{
			//if(_boundMaterial != NULL)
			//	_boundMaterial->get_effect()->reset_state();
			//_effect->set_state(mode);
			_effect->bind(mode);
			for(ParameterMap::const_iterator pItr = _params.begin(); pItr != _params.end(); ++pItr)
				_effect->set_parameter(pItr->first, pItr->second, mode);
			_boundMaterial = this;
			_boundMode = mode;
// #if defined(_DEBUG)
// 			if(!_effect->validate_program())
// 			{
// 				std::cout << _effect->get_last_error() << std::endl;
// 				assert(false);
// 			}
// #endif
		}
	}

	static void unbind()
	{
		//if(_boundMaterial != NULL)
		//{
			//if(_boundMaterial != NULL)
			//	_boundMaterial->get_effect()->reset_state();
			_boundMaterial = NULL;
		//}
		effect::Effect::unbind();
		//effect::Effect::reset_state();
	}


private:
	static const Material* _boundMaterial;
	static effect::Effect::EffectMode::type _boundMode;

	effect::Effect::ptr _effect;
	ParameterMap _params;
};

}

#endif // _SCENE_MATERIAL_H
