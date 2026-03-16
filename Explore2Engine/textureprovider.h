#if !defined(__EXPLORE2_TEXTURE_PROVIDER_H__)
#define __EXPLORE2_TEXTURE_PROVIDER_H__


#include <vector>
#include "Math/vector3.hpp"
#include "GLBase/texture.hpp"

#include "Math/sse_utils.hpp"

namespace explore2 {;

struct PlanetTexture
{
	typedef std::shared_ptr< PlanetTexture > ptr;

	typedef math::Vector3<unsigned char> Colour3b;

	void set_texture(glbase::Texture::ptr texture);

	glbase::Texture::ptr get_texture() const;
	virtual const Colour3b& get_colour() const;
	
	virtual void get_texture_weights(const math::SSETraits<float>::Vector& heights, 
		const math::SSETraits<float>::Vector& polarAngles, 
		const math::SSETraits<float>::Vector& verticalAngles,
		std::vector<float>& weights) const = 0;

	virtual PlanetTexture* clone() const = 0;

protected:
	void calculate_colour();
	void set_colour(const Colour3b& colour) { _colour = colour; }

private:
	glbase::Texture::ptr _texture;
	Colour3b _colour;
};

struct PlanetTextureBlender
{
	typedef std::shared_ptr<PlanetTextureBlender> ptr;

	PlanetTextureBlender() {}
	PlanetTextureBlender(const PlanetTextureBlender& from, bool deepCopy = false);

	void add_texture(PlanetTexture::ptr tex);
	int get_textures_count() const { return (int)_textures.size(); }
	PlanetTexture::ptr get_texture(int idx) const { return _textures[idx]; }

	void get_texture_weights(const math::SSETraits<float>::Vector& heights, 
		const math::SSETraits<float>::Vector& polarAngles, 
		const math::SSETraits<float>::Vector& verticalAngles,
		std::vector<std::vector<float>>& weights) const;
	void get_texture_colours(const math::SSETraits<float>::Vector& heights, 
		const math::SSETraits<float>::Vector& polarAngles, 
		const math::SSETraits<float>::Vector& verticalAngles,
		std::vector<unsigned char>& colourData) const;

	PlanetTexture::Colour3b get_water_colour() const 
	{
		return PlanetTexture::Colour3b(0, 0, 255);
	}
private:
	std::vector< PlanetTexture::ptr > _textures;
};

}

#endif // __EXPLORE2_TEXTURE_PROVIDER_H__