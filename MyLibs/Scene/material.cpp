#include "material.hpp"

namespace scene {;

const Material* Material::_boundMaterial = NULL;
effect::Effect::EffectMode::type Material::_boundMode;

}