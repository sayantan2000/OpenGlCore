#pragma once
#include<iostream>
#include<vector>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include"Enums.h";


#define M_E         2.71828182845904523536028747135266250   /* e */
#define M_LOG2E     1.44269504088896340735992468100189214   /* log 2e */
#define M_LOG10E    0.434294481903251827651128918916605082  /* log 10e */
#define M_LN2       0.693147180559945309417232121458176568  /* log e2 */
#define M_LN10      2.30258509299404568401799145468436421   /* log e10 */
#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2 */
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4 */
#define M_1_PI      0.318309886183790671537767526745028724  /* 1/pi */
#define M_2_PI      0.636619772367581343075535053490057448  /* 2/pi */
#define M_2_SQRTPI  1.12837916709551257389615890312154517   /* 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880168872420969808   /* sqrt(2) */
#define M_SQRT1_2   0.707106781186547524400844362104849039  /* 1/sqrt(2) */
class Primitive
{
private:
	GLfloat Pyramid[176] =
	{
		//trinagles
		//// cordinates										//colors                       
		//-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,			0.3f,0.2f,.5f,											 // Lower left corner
		//0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,				0.3f,0.1f,.5f,											 // Lower right corner
		//0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f,			0.3f,0.7f,.4f,											 // Upper corner
		//-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f,			0.3f,0.5f,.2f,											 // Inner left
		//0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f,			0.3f,0.2f,.5f,											 // Inner right
		//0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f															 // Inner down


		//square
		 // COORDINATES / COLORS / TexCoord  //
		//     COORDINATES     /        COLORS          /    TexCoord   /        NORMALS       //
		-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side
		-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
		 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
		 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side

		-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
		-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
		 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,     -0.8f, 0.5f,  0.0f, // Left Side

		-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
		 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
		 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f, -0.8f, // Non-facing side

		 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
		 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
		 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.8f, 0.5f,  0.0f, // Right side

		 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
		-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
		 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f,  0.8f  // Facing side
	};
	GLuint PyramiDIndices[18] = {
		0, 1, 2, // Bottom side
		0, 2, 3, // Bottom side
		4, 6, 5, // Left side
		7, 9, 8, // Non-facing side
		10, 12, 11, // Right side
		13, 15, 14 // Facing side
	};
public:

	std::vector<GLfloat> vertices;
	std::vector<GLuint> Indices;
	Primitive(PrimitiveType a_type);

	void GenerateSphere();
	void GeneratePyramid();

	int VerticesStride() const;
	int TriangleStride() const;


};


