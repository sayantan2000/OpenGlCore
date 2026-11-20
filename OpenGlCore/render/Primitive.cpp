#include "Primitive.h"

Primitive::Primitive(PrimitiveType a_type)
{
	switch (a_type)
	{
	case sphere:
		GenerateSphere();
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

void Primitive::GenerateSphere()
{

	// Position-only sphere used as a light-source marker (rendered with Light.vert,
	// which binds a single 3-float position attribute). Generated straight into the
	// member vectors so the layout stays correct if the segment/stack counts change.
	const float radius = 0.5f;
	const int numSegments = 50;
	const int numStacks = 50;

	vertices.clear();
	Indices.clear();
	vertices.reserve((numStacks + 1) * (numSegments + 1) * 3);
	Indices.reserve(numStacks * numSegments * 6);

	for (int stack = 0; stack <= numStacks; ++stack) {
		for (int segment = 0; segment <= numSegments; ++segment) {
			float theta = stack * M_PI / numStacks;
			float phi = segment * 2 * M_PI / numSegments;

			vertices.push_back(radius * sin(theta) * cos(phi)); // x
			vertices.push_back(radius * sin(theta) * sin(phi)); // y
			vertices.push_back(radius * cos(theta));            // z
		}
	}

	for (int stack = 0; stack < numStacks; ++stack) {
		for (int segment = 0; segment < numSegments; ++segment) {
			GLuint topLeft = stack * (numSegments + 1) + segment;
			GLuint bottomLeft = (stack + 1) * (numSegments + 1) + segment;

			Indices.push_back(topLeft);
			Indices.push_back(bottomLeft);
			Indices.push_back(topLeft + 1);

			Indices.push_back(topLeft + 1);
			Indices.push_back(bottomLeft);
			Indices.push_back(bottomLeft + 1);
		}
	}
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

int Primitive::VertexBytes() const
{
	return vertices.size() * 4;
}

int Primitive::IndexBytes() const {
	return Indices.size() * 4;
}
