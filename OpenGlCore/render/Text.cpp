#include "Text.h"
#include "GLUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

Text::Text(const char* fontPath, unsigned int pixelHeight, int screenWidth, int screenHeight)
	: shader("./Shader/text.vert", "./Shader/text.frag")
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		throw std::runtime_error("FreeType: could not initialize library");

	FT_Face face;
	if (FT_New_Face(ft, fontPath, 0, &face))
	{
		FT_Done_FreeType(ft);
		std::string msg = std::string("FreeType: failed to load font: ") + fontPath;
		std::cerr << "[TEXT ERROR] " << msg << std::endl;
		throw std::runtime_error(msg);
	}

	FT_Set_Pixel_Sizes(face, 0, pixelHeight);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // glyph bitmaps are tightly packed (1-byte rows)

	// Rasterize the first 128 ASCII glyphs, one texture each.
	for (unsigned char c = 0; c < 128; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			continue;

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
			face->glyph->bitmap.width, face->glyph->bitmap.rows,
			0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Character ch = {
			tex,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			(GLuint)face->glyph->advance.x
		};
		characters.insert({ (char)c, ch });
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Dynamic quad: 6 vertices * (vec2 pos, vec2 uv), rewritten per glyph.
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	SetScreenSize(screenWidth, screenHeight);
	glCheckError();
}

Text::~Text()
{
	for (auto& kv : characters)
		glDeleteTextures(1, &kv.second.textureID);
	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
}

void Text::SetScreenSize(int screenWidth, int screenHeight)
{
	// Pixel-space orthographic projection with origin at bottom-left.
	projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
}

void Text::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color)
{
	// Text is a 2D overlay: alpha-blend the glyph coverage and ignore depth.
	GLboolean blendWas = glIsEnabled(GL_BLEND);
	GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	shader.Activate();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shader.ID, "text"), 0);
	glBindVertexArray(vao);

	for (char c : text)
	{
		auto it = characters.find(c);
		if (it == characters.end())
			continue;
		const Character& ch = it->second;

		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;
		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		float vertices[6][4] = {
			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos,     ypos,     0.0f, 1.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },
			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },
			{ xpos + w, ypos + h, 1.0f, 0.0f }
		};

		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += (ch.advance >> 6) * scale; // advance is in 1/64 pixels
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Restore previous state.
	if (!blendWas) glDisable(GL_BLEND);
	if (depthWas) glEnable(GL_DEPTH_TEST);
}
