#include "ProjectRegistry.h"

#include <json/json.hpp>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hub {

static std::string NowIso()
{
	std::time_t t = std::time(nullptr);
	char buf[32]; std::tm tmv{};
#if defined(_WIN32)
	localtime_s(&tmv, &t);
#else
	tmv = *std::localtime(&t);
#endif
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
	return buf;
}

std::string ProjectRegistry::RegistryFilePath()
{
	std::string base;
#if defined(_WIN32)
	char* v = nullptr; size_t n = 0;
	if (_dupenv_s(&v, &n, "LOCALAPPDATA") == 0 && v) { base = v; free(v); }
#endif
	if (base.empty())
	{
		char* home = nullptr; size_t n = 0;
#if defined(_WIN32)
		if (_dupenv_s(&home, &n, "USERPROFILE") == 0 && home) { base = home; free(home); }
#else
		const char* h = std::getenv("HOME"); if (h) base = h;
#endif
	}
	if (base.empty()) base = ".";
	return (fs::path(base) / "OpenGlCoreHub" / "projects.json").string();
}

void ProjectRegistry::Load()
{
	entries.clear();
	std::ifstream in(RegistryFilePath());
	if (!in) return;
	try
	{
		json j; in >> j;
		for (const auto& e : j.value("projects", json::array()))
		{
			ProjectEntry pe;
			pe.name = e.value("name", std::string());
			pe.path = e.value("path", std::string());
			pe.lastOpened = e.value("lastOpened", std::string());
			if (!pe.path.empty()) entries.push_back(std::move(pe));
		}
	}
	catch (...) {}
}

void ProjectRegistry::Save() const
{
	std::error_code ec;
	fs::create_directories(fs::path(RegistryFilePath()).parent_path(), ec);
	json j; j["projects"] = json::array();
	for (const ProjectEntry& e : entries)
		j["projects"].push_back({ {"name", e.name}, {"path", e.path}, {"lastOpened", e.lastOpened} });
	std::ofstream out(RegistryFilePath(), std::ios::trunc);
	if (out) out << j.dump(2);
}

void ProjectRegistry::Touch(const std::string& path, const std::string& name)
{
	entries.erase(std::remove_if(entries.begin(), entries.end(),
		[&](const ProjectEntry& e) { return e.path == path; }), entries.end());
	entries.insert(entries.begin(), ProjectEntry{ name, path, NowIso() });
	if (entries.size() > 50) entries.resize(50);
	Save();
}

void ProjectRegistry::Remove(const std::string& path)
{
	entries.erase(std::remove_if(entries.begin(), entries.end(),
		[&](const ProjectEntry& e) { return e.path == path; }), entries.end());
	Save();
}


} // namespace hub
