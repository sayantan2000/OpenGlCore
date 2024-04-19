#include "Primitive.h"

Primitive::Primitive(PrimitiveType a_type)
{
	switch (a_type)
	{
	case sphere:
		GenerateSphere();
	case pyramid:
		GeneratePyramid();
		break;
	case Cube:
		break;
	case Plane:
		break;
	default:
		break;
	}
}

void Primitive::GenerateSphere()
{

	float radius = 0.5f;
	int numSegments = 50;
	int numStacks = 50;
	GLfloat Vertices[7803];
	GLuint indices[15000];
	for (int stack = 0; stack <= numStacks; ++stack) {
		for (int segment = 0; segment <= numSegments; ++segment) {
			float theta = stack * M_PI / numStacks;
			float phi = segment * 2 * M_PI / numSegments;

			GLfloat x = radius * sin(theta) * cos(phi);
			GLfloat y = radius * sin(theta) * sin(phi);
			GLfloat z = radius * cos(theta);

			int iter = 3 * (stack * (numSegments + 1) + segment);
			Vertices[iter] = x;
			Vertices[iter + 1] = y;
			Vertices[iter + 2] = z;
			/*vertices[index + 1] = y;
			vertices[index + 2] = z;*/
		}
	}
	int index = 0;
	for (int stack = 0; stack < numStacks; ++stack) {
		for (int segment = 0; segment < numSegments; ++segment) {
			indices[index++] = stack * (numSegments + 1) + segment;
			indices[index++] = (stack + 1) * (numSegments + 1) + segment;
			indices[index++] = stack * (numSegments + 1) + segment + 1;

			indices[index++] = stack * (numSegments + 1) + segment + 1;
			indices[index++] = (stack + 1) * (numSegments + 1) + segment;
			indices[index++] = (stack + 1) * (numSegments + 1) + segment + 1;
		}
	}
	for (size_t i = 0; i < sizeof(Vertices) / sizeof(GLfloat); i++)
	{
		vertices.push_back(Vertices[i]);
	}
	for (size_t i = 0; i < sizeof(indices) / sizeof(GLuint); i++)
	{
		Indices.push_back(indices[i]);
	}

	//for (auto i : Vertices) {
	//	vertices->push_back(i);
	//}
	//for (auto i : indices) {
	//	Indices->push_back(i);
	//}


}


void Primitive::GeneratePyramid()
{
	for (auto i : Pyramid) {
		vertices.push_back(i);
	}
	for (auto i : PyramiDIndices) {
		Indices.push_back(i);
	}
}

int Primitive::VerticesStride() const
{
	return vertices.size() * 4;
}

int Primitive::TriangleStride() const {
	return Indices.size() * 4;
}
