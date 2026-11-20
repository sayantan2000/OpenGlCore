#ifndef PICKING_H
#define PICKING_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>

// Mouse-ray object picking against per-object AABBs. The scene renders fullscreen, so
// screen coordinates map directly to the framebuffer. Rays are transformed into each
// object's local space and tested against its local AABB (handling rotation/scale), then
// the hit point is returned to world space so the nearest object to the camera wins.
namespace Picking
{
	struct Ray
	{
		glm::vec3 origin;
		glm::vec3 dir;   // normalized, world space
	};

	// mouseX/mouseY are in pixels with origin at the top-left (GLFW/ImGui convention).
	inline Ray ScreenPointToRay(float mouseX, float mouseY, int screenW, int screenH,
		const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos)
	{
		float ndcX = (2.0f * mouseX) / (float)screenW - 1.0f;
		float ndcY = 1.0f - (2.0f * mouseY) / (float)screenH; // flip Y
		glm::vec4 clipNear(ndcX, ndcY, -1.0f, 1.0f);
		glm::vec4 clipFar(ndcX, ndcY, 1.0f, 1.0f);

		glm::mat4 invVP = glm::inverse(proj * view);
		glm::vec4 worldNear = invVP * clipNear;
		glm::vec4 worldFar = invVP * clipFar;
		worldNear /= worldNear.w;
		worldFar /= worldFar.w;

		Ray r;
		r.origin = camPos;
		r.dir = glm::normalize(glm::vec3(worldFar - worldNear));
		return r;
	}

	// Slab test. Returns true on hit and writes the nearest positive entry parameter `tHit`.
	inline bool RayAABB(const glm::vec3& o, const glm::vec3& d,
		const glm::vec3& bmin, const glm::vec3& bmax, float& tHit)
	{
		float tmin = -std::numeric_limits<float>::max();
		float tmax = std::numeric_limits<float>::max();
		for (int i = 0; i < 3; ++i)
		{
			if (std::abs(d[i]) < 1e-8f)
			{
				if (o[i] < bmin[i] || o[i] > bmax[i]) return false; // parallel & outside slab
			}
			else
			{
				float inv = 1.0f / d[i];
				float t1 = (bmin[i] - o[i]) * inv;
				float t2 = (bmax[i] - o[i]) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = glm::max(tmin, t1);
				tmax = glm::min(tmax, t2);
				if (tmin > tmax) return false;
			}
		}
		tHit = (tmin >= 0.0f) ? tmin : tmax; // use exit param if the origin is inside
		return tHit >= 0.0f;
	}

	// Tests a world-space ray against an AABB defined in `model` local space. On hit, writes
	// the world-space distance from the ray origin to the hit point.
	inline bool RayObjectAABB(const Ray& ray, const glm::mat4& model,
		const glm::vec3& localMin, const glm::vec3& localMax, float& worldDist)
	{
		glm::mat4 inv = glm::inverse(model);
		glm::vec3 lo = glm::vec3(inv * glm::vec4(ray.origin, 1.0f));
		glm::vec3 ld = glm::vec3(inv * glm::vec4(ray.dir, 0.0f)); // not normalized (keeps scale)
		float tLocal;
		if (!RayAABB(lo, ld, localMin, localMax, tLocal)) return false;
		glm::vec3 localHit = lo + ld * tLocal;
		glm::vec3 worldHit = glm::vec3(model * glm::vec4(localHit, 1.0f));
		worldDist = glm::length(worldHit - ray.origin);
		return true;
	}
}

#endif
