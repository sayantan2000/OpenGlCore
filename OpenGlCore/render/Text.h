#ifndef TEXT_CLASS_H
#define TEXT_CLASS_H

#include<map>
#include<string>
#include<glm/glm.hpp>
#include<glad/glad.h>
#include"shaderClass.h"

namespace ogl {

// Renders 2D screen-space text using FreeType-rasterized glyph textures and
// textured quads. One Text owns a font atlas (one texture per ASCII glyph), a
// dynamic quad VAO/VBO, and the text shader. Non-copyable (owns GL resources).
class Text
{
public:
	// fontPath: path to a .ttf; pixelHeight: glyph raster height in px;
	// screen size: used to build the orthographic (pixel-space) projection.
	Text(const char* fontPath, unsigned int pixelHeight, int screenWidth, int screenHeight);
	~Text();
	Text(const Text&) = delete;
	Text& operator=(const Text&) = delete;

	// Draws text with its baseline left origin at (x, y) in pixels (origin bottom-left).
	void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
	// Update when the framebuffer size changes so text stays pixel-accurate.
	void SetScreenSize(int screenWidth, int screenHeight);

private:
	struct Character
	{
		GLuint textureID;   // glyph texture
		glm::ivec2 size;    // glyph size in px
		glm::ivec2 bearing; // offset from baseline to top-left
		GLuint advance;     // horizontal advance (1/64 px)
	};
	std::map<char, Character> characters;
	Shader shader;
	GLuint vao = 0;
	GLuint vbo = 0;
	glm::mat4 projection = glm::mat4(1.0f);
};

} // namespace ogl
#endif
