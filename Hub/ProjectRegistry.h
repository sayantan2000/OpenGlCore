#ifndef HUB_PROJECT_REGISTRY_H
#define HUB_PROJECT_REGISTRY_H

#include <string>
#include <vector>

namespace hub {

// One entry in the recent-projects list.
struct ProjectEntry
{
	std::string name;
	std::string path;          // absolute project root
	std::string lastOpened;    // ISO-ish timestamp
};

// Persists the list of recently opened projects to a per-user JSON file
// (%LOCALAPPDATA%/OpenGlCoreHub/projects.json on Windows). This is the Hub's own state and has
// nothing to do with the engine's per-project asset database.
class ProjectRegistry
{
public:
	void Load();                 // reads the registry file (safe if missing)
	void Save() const;           // writes it (creates the folder)

	const std::vector<ProjectEntry>& Entries() const { return entries; }

	// Adds or moves a project to the front (most-recent-first), de-duplicated by path, and saves.
	void Touch(const std::string& path, const std::string& name);
	// Removes a project from the list WITHOUT deleting its files, and saves.
	void Remove(const std::string& path);

	static std::string RegistryFilePath();

private:
	std::vector<ProjectEntry> entries;
};


} // namespace hub
#endif
