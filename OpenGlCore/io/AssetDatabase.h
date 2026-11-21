#ifndef ASSET_DATABASE_H
#define ASSET_DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "Guid.h"

namespace ogl {

// The kind of a project asset, inferred from its file extension.
enum class AssetType
{
	Unknown, Folder, Texture, Model, Shader, Audio, Material, Scene, Prefab, Script
};

const char* AssetTypeName(AssetType t);
AssetType   AssetTypeFromExtension(const std::string& ext);

// One indexed asset. `path` is relative to the project's Assets/ folder (forward slashes) and is
// the human-readable location; `guid` is the stable identity that survives moves/renames (it is
// stored in a sibling `<file>.meta`). References elsewhere (materials, scenes) use the GUID.
struct AssetRecord
{
	Guid guid;
	std::string path;          // relative to Assets/, '/'-separated
	AssetType type = AssetType::Unknown;
	std::int64_t mtime = 0;    // last-write time (for change detection / reimport)
	bool missing = false;      // true if the meta exists but the asset file is gone
};

// A Unity-style asset database for one open project. It scans the project's Assets/ tree, gives
// every asset a stable GUID via a `.meta` sidecar, and maintains bidirectional GUID<->path maps so
// code can resolve a serialized GUID to a current path (and vice-versa) and so references survive
// moving/renaming files. It owns no GPU resources — it is the *index*; the AssetManager still does
// the actual loading/caching by path.
//
// Not a singleton: the Application owns one. Pure filesystem + JSON; safe to unit-test headlessly.
class AssetDatabase
{
public:
	// Points the database at a project root (the folder containing Assets/). Creates the standard
	// project directory skeleton if missing, then scans. Returns false if the root is unusable.
	bool Open(const std::string& projectRoot);
	bool IsOpen() const { return !root.empty(); }

	const std::string& ProjectRoot() const { return root; }
	std::string AssetsDir() const;                     // <root>/Assets
	std::string AbsolutePath(const std::string& relPath) const; // <root>/Assets/<relPath>

	// (Re)scan the Assets/ tree: assign GUIDs to new files (writing .meta), update paths for files
	// that moved (matched by the GUID in their .meta), mark vanished assets missing, and refresh
	// modification times. Idempotent. Returns the number of records after scanning.
	int Refresh();

	// Lookups. Return nullptr / invalid / empty when not found.
	const AssetRecord* Find(const Guid& guid) const;
	const AssetRecord* FindByPath(const std::string& relPath) const;
	Guid PathToGuid(const std::string& relPath) const;
	std::string GuidToPath(const Guid& guid) const;               // relative
	std::string GuidToAbsolutePath(const Guid& guid) const;       // absolute

	// All records (optionally filtered by type), and the immediate children of a folder (relative
	// folder path, "" = Assets root) for the Project browser.
	const std::vector<AssetRecord>& All() const { return records; }
	std::vector<const AssetRecord*> InFolder(const std::string& relFolder) const;

	// Creates (or ensures) the standard project folder structure under the root.
	static bool CreateProjectSkeleton(const std::string& projectRoot, std::string* err = nullptr);

	// Registers a brand-new asset file the engine just wrote (e.g. a material) so it gets a GUID
	// and a .meta immediately without a full Refresh. Returns its GUID (existing one if already
	// tracked).
	Guid RegisterNewAsset(const std::string& relPath);

private:
	void ScanDir(const std::string& absDir);
	Guid EnsureMeta(const std::string& absFile, AssetType type, std::int64_t mtime);
	static std::string MetaPathFor(const std::string& absFile) { return absFile + ".meta"; }

	std::string root;                                  // project root (contains Assets/)
	std::vector<AssetRecord> records;
	std::unordered_map<Guid, size_t> byGuid;           // guid -> index into records
	std::unordered_map<std::string, size_t> byPath;    // relPath -> index into records
};


} // namespace ogl
#endif
