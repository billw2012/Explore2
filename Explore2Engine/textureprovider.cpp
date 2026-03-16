
#include "textureprovider.h"

namespace explore2 {;

void PlanetTexture::set_texture(glbase::Texture::ptr texture)
{
	assert(texture);
	assert(texture->type() == GL_UNSIGNED_BYTE);
	assert(texture->components() == 3 || texture->components() == 4);

	_texture = texture;

	calculate_colour();
}

glbase::Texture::ptr PlanetTexture::get_texture() const
{
	return _texture;
}

const PlanetTexture::Colour3b& PlanetTexture::get_colour() const
{
	return _colour;
}

void PlanetTexture::calculate_colour()
{
	std::vector<unsigned char> data(_texture->size());
	_texture->bind();
	_texture->getData(&data[0]);
	_texture->unbind();

	math::Vector3d averageColor;
	size_t textureElements = _texture->width() * _texture->height();
	for(size_t j = 0, offs=0; j < textureElements; ++j, offs+=_texture->components())
	{
		for(unsigned int k = 0; k < _texture->components() && k < 3; ++k)
			averageColor(k) += data[offs + k]/256.0f;
	}
	averageColor /= textureElements;
	_colour = Colour3b(averageColor * 256);
}
//void PlanetTexture::get_texture_weights(const std::vector<float>& heights, 
//										const std::vector<float>& polarAngles, 
//										const std::vector<float>& verticalAngles,
//										std::vector<float>& weights) const
//{
//
//}

PlanetTextureBlender::PlanetTextureBlender(const PlanetTextureBlender& from, bool deepCopy)
{
	if(deepCopy)
	{
		_textures.resize(from._textures.size());
		for(size_t idx = 0; idx < from._textures.size(); ++idx)
		{
			_textures[idx].reset(from._textures[idx]->clone());
		}
	}
	else
	{
		_textures.insert(_textures.begin(), from._textures.begin(), from._textures.end());
	}
}

void PlanetTextureBlender::add_texture(PlanetTexture::ptr tex)
{
	_textures.push_back(tex);
}

void PlanetTextureBlender::get_texture_weights(const math::SSETraits<float>::Vector& heights, 
											   const math::SSETraits<float>::Vector& polarAngles, 
											   const math::SSETraits<float>::Vector& verticalAngles,
											   std::vector<std::vector<float>>& weights) const
{
	weights.resize(_textures.size());

	for(size_t idx = 0; idx < _textures.size(); ++idx)
	{
		_textures[idx]->get_texture_weights(heights, polarAngles, verticalAngles, weights[idx]);
	}
}

void PlanetTextureBlender::get_texture_colours(const math::SSETraits<float>::Vector& heights, 
											   const math::SSETraits<float>::Vector& polarAngles, 
											   const math::SSETraits<float>::Vector& verticalAngles,
											   std::vector<unsigned char>& colourData) const
{
	std::vector<std::vector<float>> weights;

	get_texture_weights(heights, polarAngles, verticalAngles, weights);

	colourData.resize(heights.size() * 3);

	memset(&(colourData[0]), 0, colourData.size());

	for(size_t idx = 0; idx < weights.size(); ++idx)
	{
		std::vector<float>& textureWeights = weights[idx];
		math::Vector3f textureColour(_textures[idx]->get_colour());
		//*colour = PlanetTexture::Colour3b::Zero;
		for(size_t wIdx = 0; wIdx < textureWeights.size(); ++wIdx)
		{
			PlanetTexture::Colour3b* colour = reinterpret_cast<PlanetTexture::Colour3b*>(&colourData[wIdx*3]);
			*colour = PlanetTexture::Colour3b(math::Vector3f(*colour) * (1 - textureWeights[wIdx]) + textureColour * textureWeights[wIdx]);
		}
	}
}

}