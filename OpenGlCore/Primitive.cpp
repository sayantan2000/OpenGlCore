#include "Primitive.h"

Primitive::Primitive(PrimitiveType a_type)
{
	switch (a_type)
	{
	case sphere:
		break;
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

void Primitive::GeneratePyramid()
{

	vertices = new std::vector<GLfloat>;
	Indices = new std::vector<GLuint>();

	for (auto i : Pyramid) {
		vertices->push_back(i);
	}
	for (auto i : PyramiDIndices) {
		Indices->push_back(i);
	}
}
