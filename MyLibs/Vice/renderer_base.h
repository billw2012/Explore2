#pragma once

namespace vice {;

struct RendererBase
{
	void render_rect(int x, int y, int width, int height);
	void render_text(int x, int y, int width, int height, const std::string& str);
};

}