#include "ProjectTemplate.h"

#include <json/json.hpp>
#include <filesystem>
#include <fstream>
#include <ctime>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hub {

const char* TemplateName(Template t)
{
	switch (t) { case Template::Starter: return "Starter 3D"; default: return "Blank 3D"; }
}

static std::string NowIso()
{
	std::time_t t = std::time(nullptr);
	char buf[32];
	std::tm tmv{};
#if defined(_WIN32)
	localtime_s(&tmv, &t);
#else
	tmv = *std::localtime(&t);
#endif
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
	return buf;
}

bool CreateProject(const std::string& location, const std::string& name, Template tmpl,
	const std::string& engineVersion, std::string* outProjectPath, std::string* err)
{
	if (name.empty()) { if (err) *err = "project name is empty"; return false; }
	fs::path proj = fs::path(location) / name;
	std::error_code ec;

	if (fs::exists(proj) && fs::is_directory(proj) && !fs::is_empty(proj, ec))
	{ if (err) *err = "target folder already exists and is not empty"; return false; }

	static const char* dirs[] = {
		"Assets", "Assets/Scenes", "Assets/Materials", "Assets/Textures", "Assets/Models",
		"Assets/Shaders", "Assets/Scripts", "Assets/Audio", "Assets/Prefabs", "Assets/Plugins",
		"Library", "Temp", "Build", "ProjectSettings"
	};
	for (const char* d : dirs)
	{
		fs::create_directories(proj / d, ec);
		if (ec) { if (err) *err = std::string("failed to create ") + d + ": " + ec.message(); return false; }
	}

	// ProjectSettings/project.json — the editor reads `template` to seed the first scene.
	json ps;
	ps["name"] = name;
	ps["engineVersion"] = engineVersion;
	ps["template"] = (tmpl == Template::Starter) ? "starter" : "blank";
	ps["created"] = NowIso();
	{
		std::ofstream out((proj / "ProjectSettings" / "project.json").string(), std::ios::trunc);
		if (!out) { if (err) *err = "failed to write project.json"; return false; }
		out << ps.dump(2);
	}

	// Empty scene placeholder (the editor fills/overwrites it on first open per the template).
	{
		json scene; scene["version"] = 1; scene["objects"] = json::array();
		std::ofstream out((proj / "Assets" / "Scenes" / "Main.scene").string(), std::ios::trunc);
		if (out) out << scene.dump(2);
	}

	if (outProjectPath) *outProjectPath = proj.string();
	return true;
}

bool IsValidProject(const std::string& path)
{
	std::error_code ec;
	return fs::exists(fs::path(path) / "Assets", ec)
		&& fs::exists(fs::path(path) / "ProjectSettings" / "project.json", ec);
}

std::string ReadProjectName(const std::string& path)
{
	std::ifstream in((fs::path(path) / "ProjectSettings" / "project.json").string());
	if (in)
	{
		try { json j; in >> j; std::string n = j.value("name", std::string()); if (!n.empty()) return n; }
		catch (...) {}
	}
	return fs::path(path).filename().string();
}


} // namespace hub
