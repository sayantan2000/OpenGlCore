#ifndef HUB_PROJECT_TEMPLATE_H
#define HUB_PROJECT_TEMPLATE_H

#include <string>

namespace hub {

// Project templates the Hub can scaffold. The editor reads the chosen template from the generated
// ProjectSettings/project.json and populates the initial scene accordingly on first open.
enum class Template { Blank, Starter };

const char* TemplateName(Template t);

// Creates a complete project at <location>/<name>: the standard directory skeleton, a
// ProjectSettings/project.json (name, engine version, template, timestamp), and an empty
// Assets/Scenes/Main.scene placeholder. Returns false (with *err) on failure or if the target
// already exists and is non-empty.
bool CreateProject(const std::string& location, const std::string& name, Template tmpl,
	const std::string& engineVersion, std::string* outProjectPath, std::string* err);

// True if `path` looks like a valid engine project (has Assets/ and ProjectSettings/project.json).
bool IsValidProject(const std::string& path);

// Reads the display name from a project's project.json (falls back to the folder name).
std::string ReadProjectName(const std::string& path);


} // namespace hub
#endif
