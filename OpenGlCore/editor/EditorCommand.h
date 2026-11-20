#ifndef EDITOR_COMMAND_H
#define EDITOR_COMMAND_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "Scene.h"
#include "GameObject.h"

namespace ogl {

// -----------------------------------------------------------------------------------------
// Editor undo/redo (Command pattern).
//
// Commands operate on the Scene by stable object id, so they survive index changes and
// object deletion/restoration. Property edits use full-GameObject value snapshots
// (before/after): a GameObject is cheap to copy (strings + value components; geometry is
// referenced by non-owning pointer, so copies share the same Mesh/Model/texture handles,
// keeping resource references valid after undo/redo).
//
// Convention: a command is pushed AFTER its effect has already been applied to the scene
// by the editor. History::Push therefore does not call Redo(); Undo()/Redo() move between
// the stacks and (re)apply the stored state.
// -----------------------------------------------------------------------------------------
class EditorCommand
{
public:
	virtual ~EditorCommand() = default;
	virtual void Undo(Scene& scene) = 0;
	virtual void Redo(Scene& scene) = 0;
	virtual const std::string& Name() const = 0;
};

// Property change on an existing object (transform / material / light / name / active).
class ModifyCommand : public EditorCommand
{
public:
	ModifyCommand(std::uint32_t id, const GameObject& before, const GameObject& after, std::string name)
		: objId(id), beforeState(before), afterState(after), label(std::move(name)) {}

	void Undo(Scene& scene) override { Apply(scene, beforeState); }
	void Redo(Scene& scene) override { Apply(scene, afterState); }
	const std::string& Name() const override { return label; }

private:
	void Apply(Scene& scene, const GameObject& state)
	{
		if (GameObject* go = scene.FindById(objId))
		{
			std::uint32_t keepId = go->id;
			*go = state;
			go->id = keepId;               // never let the snapshot change identity
			scene.SelectById(keepId);
		}
	}
	std::uint32_t objId;
	GameObject beforeState, afterState;
	std::string label;
};

// Object creation (menu / duplicate). Redo (re)inserts a copy; Undo removes it.
class AddCommand : public EditorCommand
{
public:
	AddCommand(const GameObject& proto, size_t index, std::string name)
		: state(proto), insertIndex(index), label(std::move(name)) {}

	void Undo(Scene& scene) override { scene.RemoveById(state.id); }
	void Redo(Scene& scene) override
	{
		auto go = std::make_unique<GameObject>(state);
		scene.InsertAt(insertIndex, std::move(go));
		scene.SelectById(state.id);
	}
	const std::string& Name() const override { return label; }

private:
	GameObject state;
	size_t insertIndex;
	std::string label;
};

// Object deletion. Redo removes; Undo restores at the original index with the same id.
class RemoveCommand : public EditorCommand
{
public:
	RemoveCommand(const GameObject& removed, size_t index, std::string name)
		: state(removed), originalIndex(index), label(std::move(name)) {}

	void Undo(Scene& scene) override
	{
		auto go = std::make_unique<GameObject>(state);
		scene.InsertAt(originalIndex, std::move(go));
		scene.SelectById(state.id);
	}
	void Redo(Scene& scene) override { scene.RemoveById(state.id); }
	const std::string& Name() const override { return label; }

private:
	GameObject state;
	size_t originalIndex;
	std::string label;
};

// Undo/redo stacks with a bounded depth.
class CommandHistory
{
public:
	void Push(std::unique_ptr<EditorCommand> cmd)
	{
		redoStack.clear();
		undoStack.push_back(std::move(cmd));
		if (undoStack.size() > capacity)
			undoStack.erase(undoStack.begin());
	}

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	// Returns the name of the applied command (empty if nothing to do).
	std::string Undo(Scene& scene)
	{
		if (undoStack.empty()) return {};
		auto cmd = std::move(undoStack.back());
		undoStack.pop_back();
		cmd->Undo(scene);
		std::string n = cmd->Name();
		redoStack.push_back(std::move(cmd));
		return n;
	}
	std::string Redo(Scene& scene)
	{
		if (redoStack.empty()) return {};
		auto cmd = std::move(redoStack.back());
		redoStack.pop_back();
		cmd->Redo(scene);
		std::string n = cmd->Name();
		undoStack.push_back(std::move(cmd));
		return n;
	}

	void Clear() { undoStack.clear(); redoStack.clear(); }

	// Recent command names, newest last (for an optional history panel).
	const std::vector<std::unique_ptr<EditorCommand>>& UndoStack() const { return undoStack; }
	const std::vector<std::unique_ptr<EditorCommand>>& RedoStack() const { return redoStack; }

private:
	std::vector<std::unique_ptr<EditorCommand>> undoStack;
	std::vector<std::unique_ptr<EditorCommand>> redoStack;
	size_t capacity = 128;
};


} // namespace ogl
#endif
