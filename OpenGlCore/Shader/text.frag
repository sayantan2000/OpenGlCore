#version 330 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;      // single-channel (RED) glyph coverage
uniform vec3 textColor;
void main()
{
    float alpha = texture(text, TexCoords).r;
    color = vec4(textColor, alpha);
}
