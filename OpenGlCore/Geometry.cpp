#include "Geometry.h"
#include <cmath>

namespace Geometry
{
	void BuildSphere(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments, float radius)
	{
		const float PI = 3.14159265358979323846f;
		vertices.clear();
		indices.clear();

		for (int y = 0; y <= segments; ++y)
		{
			for (int x = 0; x <= segments; ++x)
			{
				float xSeg = (float)x / (float)segments;
				float ySeg = (float)y / (float)segments;
				float xPos = std::cos(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
				float yPos = std::cos(ySeg * PI);
				float zPos = std::sin(xSeg * 2.0f * PI) * std::sin(ySeg * PI);

				// pos
				vertices.push_back(xPos * radius);
				vertices.push_back(yPos * radius);
				vertices.push_back(zPos * radius);
				// color (unused by PBR; white placeholder)
				vertices.push_back(1.0f);
				vertices.push_back(1.0f);
				vertices.push_back(1.0f);
				// uv
				vertices.push_back(xSeg);
				vertices.push_back(ySeg);
				// normal (unit sphere position == normal)
				vertices.push_back(xPos);
				vertices.push_back(yPos);
				vertices.push_back(zPos);
			}
		}

		// Two triangles per quad cell.
		int stride = segments + 1;
		for (int y = 0; y < segments; ++y)
		{
			for (int x = 0; x < segments; ++x)
			{
				GLuint i0 = y * stride + x;
				GLuint i1 = i0 + 1;
				GLuint i2 = i0 + stride;
				GLuint i3 = i2 + 1;

				indices.push_back(i0);
				indices.push_back(i2);
				indices.push_back(i1);

				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i3);
			}
		}
	}
}
