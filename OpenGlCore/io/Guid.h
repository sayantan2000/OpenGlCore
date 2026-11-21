#ifndef GUID_H
#define GUID_H

#include <cstdint>
#include <string>
#include <random>
#include <functional>

namespace ogl {

// A 128-bit globally-unique identifier used as a *stable* reference to a project asset. Unlike a
// filesystem path, a GUID does not change when an asset is moved or renamed (the GUID travels
// with the asset in its `.meta` sidecar), so serialized references (materials -> textures, scenes
// -> models) survive reorganizing the Assets folder. Serialized as 32 lowercase hex characters.
struct Guid
{
	std::uint64_t hi = 0;
	std::uint64_t lo = 0;

	bool IsValid() const { return hi != 0 || lo != 0; }
	bool operator==(const Guid& o) const { return hi == o.hi && lo == o.lo; }
	bool operator!=(const Guid& o) const { return !(*this == o); }

	// Random 128-bit GUID (version-4-style; uses a thread-local PRNG seeded from random_device).
	static Guid Generate()
	{
		static thread_local std::mt19937_64 rng{ std::random_device{}() };
		std::uniform_int_distribution<std::uint64_t> dist;
		Guid g; g.hi = dist(rng); g.lo = dist(rng);
		if (!g.IsValid()) g.lo = 1; // never return the all-zero (invalid) guid
		return g;
	}

	std::string ToString() const
	{
		static const char* k = "0123456789abcdef";
		std::string s(32, '0');
		for (int i = 0; i < 16; ++i)
		{
			std::uint64_t v = (i < 8) ? hi : lo;
			int shift = (7 - (i % 8)) * 8;
			unsigned byte = (unsigned)((v >> shift) & 0xFF);
			s[i * 2]     = k[(byte >> 4) & 0xF];
			s[i * 2 + 1] = k[byte & 0xF];
		}
		return s;
	}

	static Guid FromString(const std::string& s)
	{
		Guid g;
		if (s.size() != 32) return g;
		auto hex = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		std::uint64_t hi = 0, lo = 0;
		for (int i = 0; i < 16; ++i)
		{
			int h = hex(s[i * 2]), l = hex(s[i * 2 + 1]);
			if (h < 0 || l < 0) return Guid{}; // malformed -> invalid
			std::uint64_t byte = (std::uint64_t)((h << 4) | l);
			if (i < 8) hi = (hi << 8) | byte; else lo = (lo << 8) | byte;
		}
		g.hi = hi; g.lo = lo;
		return g;
	}
};


} // namespace ogl

// Hash so Guid can key unordered_map/unordered_set.
namespace std {
template <> struct hash<ogl::Guid>
{
	size_t operator()(const ogl::Guid& g) const noexcept
	{
		return std::hash<std::uint64_t>{}(g.hi) ^ (std::hash<std::uint64_t>{}(g.lo) * 1099511628211ull);
	}
};
}

#endif
