#include "Geometry.h"
#include <cmath>

namespace ogl {

namespace
{
	constexpr float PI = 3.14159265358979323846f;

	// Push one vertex in the 11-float layout (pos, white colour, uv, normal).
	inline void PushV(std::vector<GLfloat>& v,
		float px, float py, float pz, float u, float uv, float nx, float ny, float nz)
	{
		v.push_back(px); v.push_back(py); v.push_back(pz);
		v.push_back(1.0f); v.push_back(1.0f); v.push_back(1.0f);
		v.push_back(u); v.push_back(uv);
		v.push_back(nx); v.push_back(ny); v.push_back(nz);
	}
}

namespace Geometry
{
	void BuildSphere(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments, float radius)
	{
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
				PushV(vertices, xPos * radius, yPos * radius, zPos * radius, xSeg, ySeg, xPos, yPos, zPos);
			}
		}

		int stride = segments + 1;
		for (int y = 0; y < segments; ++y)
		{
			for (int x = 0; x < segments; ++x)
			{
				GLuint i0 = y * stride + x;
				GLuint i1 = i0 + 1;
				GLuint i2 = i0 + stride;
				GLuint i3 = i2 + 1;
				indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
				indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
			}
		}
	}

	void BuildCube(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
	{
		vertices.clear();
		indices.clear();

		// 6 faces, each with its own normal and 4 corners (CCW when viewed from outside).
		struct Face { float n[3]; float t[3]; float b[3]; };
		const Face faces[6] = {
			{ { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 } },  // +Z
			{ { 0, 0,-1 }, {-1, 0, 0 }, { 0, 1, 0 } },  // -Z
			{ { 1, 0, 0 }, { 0, 0,-1 }, { 0, 1, 0 } },  // +X
			{ {-1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 } },  // -X
			{ { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0,-1 } },  // +Y
			{ { 0,-1, 0 }, { 1, 0, 0 }, { 0, 0, 1 } },  // -Y
		};

		GLuint base = 0;
		for (const Face& f : faces)
		{
			// corner = n + s*t + r*b, with (s,r) in {-1,+1}
			const float su[4] = { -1,  1,  1, -1 };
			const float sv[4] = { -1, -1,  1,  1 };
			for (int c = 0; c < 4; ++c)
			{
				float px = f.n[0] + su[c] * f.t[0] + sv[c] * f.b[0];
				float py = f.n[1] + su[c] * f.t[1] + sv[c] * f.b[1];
				float pz = f.n[2] + su[c] * f.t[2] + sv[c] * f.b[2];
				PushV(vertices, px, py, pz, (su[c] + 1) * 0.5f, (sv[c] + 1) * 0.5f, f.n[0], f.n[1], f.n[2]);
			}
			indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
			indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
			base += 4;
		}
	}

	void BuildPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, int subdivisions)
	{
		vertices.clear();
		indices.clear();
		if (subdivisions < 1) subdivisions = 1;

		for (int z = 0; z <= subdivisions; ++z)
		{
			for (int x = 0; x <= subdivisions; ++x)
			{
				float fx = (float)x / subdivisions; // 0..1
				float fz = (float)z / subdivisions;
				PushV(vertices, fx * 2.0f - 1.0f, 0.0f, fz * 2.0f - 1.0f, fx, fz, 0.0f, 1.0f, 0.0f);
			}
		}
		int stride = subdivisions + 1;
		for (int z = 0; z < subdivisions; ++z)
		{
			for (int x = 0; x < subdivisions; ++x)
			{
				GLuint i0 = z * stride + x;
				GLuint i1 = i0 + 1;
				GLuint i2 = i0 + stride;
				GLuint i3 = i2 + 1;
				indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
				indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
			}
		}
	}

	void BuildCylinder(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments, float radius, float height)
	{
		vertices.clear();
		indices.clear();
		float hy = height * 0.5f;

		// Side wall (duplicated top/bottom rings with radial normals).
		GLuint sideBase = 0;
		for (int i = 0; i <= segments; ++i)
		{
			float a = (float)i / segments * 2.0f * PI;
			float cx = std::cos(a), cz = std::sin(a);
			float u = (float)i / segments;
			PushV(vertices, cx * radius, -hy, cz * radius, u, 0.0f, cx, 0.0f, cz); // bottom
			PushV(vertices, cx * radius,  hy, cz * radius, u, 1.0f, cx, 0.0f, cz); // top
		}
		for (int i = 0; i < segments; ++i)
		{
			GLuint b0 = sideBase + i * 2;
			GLuint t0 = b0 + 1;
			GLuint b1 = b0 + 2;
			GLuint t1 = b0 + 3;
			indices.push_back(b0); indices.push_back(b1); indices.push_back(t0);
			indices.push_back(t0); indices.push_back(b1); indices.push_back(t1);
		}

		// Caps (center + fan).
		auto buildCap = [&](float y, float ny)
		{
			GLuint center = (GLuint)(vertices.size() / 11);
			PushV(vertices, 0.0f, y, 0.0f, 0.5f, 0.5f, 0.0f, ny, 0.0f);
			GLuint ringStart = (GLuint)(vertices.size() / 11);
			for (int i = 0; i <= segments; ++i)
			{
				float a = (float)i / segments * 2.0f * PI;
				float cx = std::cos(a), cz = std::sin(a);
				PushV(vertices, cx * radius, y, cz * radius, cx * 0.5f + 0.5f, cz * 0.5f + 0.5f, 0.0f, ny, 0.0f);
			}
			for (int i = 0; i < segments; ++i)
			{
				GLuint r0 = ringStart + i;
				GLuint r1 = ringStart + i + 1;
				if (ny > 0.0f) { indices.push_back(center); indices.push_back(r0); indices.push_back(r1); }
				else { indices.push_back(center); indices.push_back(r1); indices.push_back(r0); }
			}
		};
		buildCap(hy, 1.0f);
		buildCap(-hy, -1.0f);
	}

	void BuildCone(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments, float radius, float height)
	{
		vertices.clear();
		indices.clear();
		float hy = height * 0.5f;

		// Side: one apex vertex per segment (so normals are per-face-ish and smooth around).
		float slant = std::sqrt(radius * radius + height * height);
		float ny = radius / slant;       // normal y component
		float nr = height / slant;       // normal radial component
		for (int i = 0; i < segments; ++i)
		{
			float a0 = (float)i / segments * 2.0f * PI;
			float a1 = (float)(i + 1) / segments * 2.0f * PI;
			float am = (a0 + a1) * 0.5f;
			float c0x = std::cos(a0), c0z = std::sin(a0);
			float c1x = std::cos(a1), c1z = std::sin(a1);
			float cmx = std::cos(am), cmz = std::sin(am);
			GLuint base = (GLuint)(vertices.size() / 11);
			PushV(vertices, 0.0f, hy, 0.0f, 0.5f, 1.0f, cmx * nr, ny, cmz * nr);               // apex
			PushV(vertices, c0x * radius, -hy, c0z * radius, (float)i / segments, 0.0f, c0x * nr, ny, c0z * nr);
			PushV(vertices, c1x * radius, -hy, c1z * radius, (float)(i + 1) / segments, 0.0f, c1x * nr, ny, c1z * nr);
			indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
		}

		// Base cap (normal -Y).
		GLuint center = (GLuint)(vertices.size() / 11);
		PushV(vertices, 0.0f, -hy, 0.0f, 0.5f, 0.5f, 0.0f, -1.0f, 0.0f);
		GLuint ringStart = (GLuint)(vertices.size() / 11);
		for (int i = 0; i <= segments; ++i)
		{
			float a = (float)i / segments * 2.0f * PI;
			float cx = std::cos(a), cz = std::sin(a);
			PushV(vertices, cx * radius, -hy, cz * radius, cx * 0.5f + 0.5f, cz * 0.5f + 0.5f, 0.0f, -1.0f, 0.0f);
		}
		for (int i = 0; i < segments; ++i)
		{
			indices.push_back(center); indices.push_back(ringStart + i + 1); indices.push_back(ringStart + i);
		}
	}

	void BuildTorus(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int ringSegments, int tubeSegments, float ringRadius, float tubeRadius)
	{
		vertices.clear();
		indices.clear();

		for (int i = 0; i <= ringSegments; ++i)
		{
			float u = (float)i / ringSegments * 2.0f * PI;
			float cu = std::cos(u), su = std::sin(u);
			for (int j = 0; j <= tubeSegments; ++j)
			{
				float v = (float)j / tubeSegments * 2.0f * PI;
				float cv = std::cos(v), sv = std::sin(v);
				float px = (ringRadius + tubeRadius * cv) * cu;
				float py = tubeRadius * sv;
				float pz = (ringRadius + tubeRadius * cv) * su;
				float nx = cv * cu, ny = sv, nz = cv * su;
				PushV(vertices, px, py, pz, (float)i / ringSegments, (float)j / tubeSegments, nx, ny, nz);
			}
		}
		int stride = tubeSegments + 1;
		for (int i = 0; i < ringSegments; ++i)
		{
			for (int j = 0; j < tubeSegments; ++j)
			{
				GLuint a = i * stride + j;
				GLuint b = (i + 1) * stride + j;
				indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
				indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
			}
		}
	}

	void BuildCapsule(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments, float radius, float cylinderHeight)
	{
		vertices.clear();
		indices.clear();
		int rings = segments;              // latitude rings per hemisphere
		int sectors = segments * 2;        // longitude
		float hc = cylinderHeight * 0.5f;

		// Build as a grid of (rows x (sectors+1)); rows go top cap -> cylinder -> bottom cap.
		// Row parameter: upper hemisphere [0..rings], then lower hemisphere [0..rings], the
		// seam rows at the equator are offset by +/-hc to insert the cylinder body.
		auto pushRing = [&](float phi, float yOffset)
		{
			float sp = std::sin(phi), cp = std::cos(phi); // phi: 0 at pole
			for (int s = 0; s <= sectors; ++s)
			{
				float theta = (float)s / sectors * 2.0f * PI;
				float ct = std::cos(theta), st = std::sin(theta);
				float nx = sp * ct, ny = cp, nz = sp * st;
				float px = radius * nx;
				float py = radius * ny + yOffset;
				float pz = radius * nz;
				PushV(vertices, px, py, pz, (float)s / sectors, 0.0f, nx, ny, nz);
			}
		};

		// Top hemisphere: phi 0..PI/2
		for (int r = 0; r <= rings; ++r)
			pushRing((float)r / rings * (PI * 0.5f), hc);
		// Bottom hemisphere: phi PI/2..PI
		for (int r = 0; r <= rings; ++r)
			pushRing(PI * 0.5f + (float)r / rings * (PI * 0.5f), -hc);

		int totalRows = (rings + 1) * 2;
		int stride = sectors + 1;
		for (int r = 0; r < totalRows - 1; ++r)
		{
			for (int s = 0; s < sectors; ++s)
			{
				GLuint a = r * stride + s;
				GLuint b = (r + 1) * stride + s;
				indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
				indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
			}
		}
	}
}

} // namespace ogl