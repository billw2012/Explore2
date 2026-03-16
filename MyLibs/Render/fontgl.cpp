
#include "fontgl.h"
#include "Scene/transform.hpp"
#include "GLBase/vertexset.hpp"
#include "GLBase/vertexspec.hpp"
#include "GLBase/triangleset.hpp"

#include "rendergl.h"

namespace render {;


FontGL::FontGL() : _scale(1.0f)
{

}

FontGL::~FontGL()
{
}

void FontGL::set_scale( float scale )
{
	_scale = scale;
}

void FontGL::load( const std::string& filename, const std::string& shader )
{
	using namespace scene;

	static const GLuint charsx = 16;
	static const GLuint charsy = 16;

	_charsX = charsx;
	_charsY = charsy;

	_count = _charsX * _charsY;

	_data.reset(new glbase::Texture());
	_data->load(filename);

	_material.reset(new scene::Material());
	effect::Effect::ptr effect(new effect::Effect());
	effect->load(shader);
	_material->set_effect(effect);
	_material->set_parameter("FontTexture", _data);

	//_base=glGenLists(_count);					// Creating 256 Display Lists
	//_data.bind();

	//std::vector<unsigned char> buf(256*256*3);
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &buf[0]);

	////glBindTexture(GL_TEXTURE_2D, texture[0]);			// Select Our Font Texture
	//for (GLuint loop = 0; loop < _count; ++loop)		// Loop Through All 256 Lists
	//{
	//	float cx=static_cast<float>(loop%charsx)/static_cast<float>(charsx);	// X Position Of Current Character
	//	float cy=static_cast<float>(loop/charsy)/static_cast<float>(charsy);	// Y Position Of Current Character

	//	glNewList(_base+loop, GL_COMPILE);				// Start Building A List
	//	glBegin(GL_QUADS);								// Use A Quad For Each Character
	//	glTexCoord2f(cx, 1-cy-charv);					// Texture Coord (Bottom Left)
	//	glVertex2i(0, 0);								// Vertex Coord (Bottom Left)
	//	glTexCoord2f(cx+charu, 1-cy-charv);				// Texture Coord (Bottom Right)
	//	glVertex2i(16, 0);								// Vertex Coord (Bottom Right)
	//	glTexCoord2f(cx+charu, 1-cy);					// Texture Coord (Top Right)
	//	glVertex2i(16, 16);								// Vertex Coord (Top Right)
	//	glTexCoord2f(cx, 1-cy);							// Texture Coord (Top Left)
	//	glVertex2i(0, 16);								// Vertex Coord (Top Left)
	//	glEnd();										// Done Building Our Quad (Character)
	//	glTranslated(16, 0, 0);							// Move To The Right Of The Character
	//	glEndList();									// Done Building The Display List
	//}
}

math::Rectanglef FontGL::get_char_uvs(char ch) const
{
	float charwid = static_cast<float>(_data->width()/_charsX);
	float charhgt = static_cast<float>(_data->height()/_charsY);

	float charu = 1.0f/_charsX;
	float charv = 1.0f/_charsY;

	ch -= 32;

	float cx=static_cast<float>(ch%_charsX)/static_cast<float>(_charsX);	// X Position Of Current Character
	float cy=static_cast<float>(ch/_charsY)/static_cast<float>(_charsY);	// Y Position Of Current Character

	return math::Rectanglef(cx, 1-cy-charv, cx + charu, 1-cy);
}

// void FontGL::print( GLint x, GLint y, const std::string& str )
// {
// //	//if(set > 1)
// //	//{
// //	//	set = 1;
// //	//}
// //
// //	//_data.bind();
// //	////glBindTexture(GL_TEXTURE_2D, texture[0]);			// Select Our Font Texture
// //	//glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
// //	//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
// //	//glPushMatrix();										// Store The Projection Matrix
// //	//glLoadIdentity();									// Reset The Projection Matrix
// //	//glOrtho(0,640,0,480,-1,1);							// Set Up An Ortho Screen
// //	//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
// //	//glPushMatrix();										// Store The Modelview Matrix
// //	//glLoadIdentity();									// Reset The Modelview Matrix
// //	//glScalef(_scale, _scale, _scale);
// //	//glTranslated(x,y,0);								// Position The Text (0,0 - Bottom Left)
// //	//glListBase(_base-32+(128*set));						// Choose The Font Set (0 or 1)
// //	//glCallLists(static_cast<GLsizei>(str.length()), GL_UNSIGNED_BYTE, &str[0]);// Write The Text To The Screen
// //	//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
// //	//glPopMatrix();										// Restore The Old Projection Matrix
// //	//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
// //	//glPopMatrix();										// Restore The Old Projection Matrix
// //	//glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
// }

}