# Architectural Decisions

> Records decisions and their rationale. Append new entries; don't rewrite history.

## ADR-0001 — Documentation-first baseline (2026-11-19)
- **Context**: Codebase had no docs (`CLAUDE.md`/`README.md` empty). A full architectural
  assessment was requested before any refactoring.
- **Decision**: Capture the *current* implementation in `docs/` + `CLAUDE.md`, and record
  the proposed refactor as an un-executed, prioritized `ROADMAP.md`. No source changed.
- **Rationale**: Lets future sessions (human or AI) understand the project without
  re-analyzing everything, and separates "what is" from "what should be".
- **Status**: Accepted. Roadmap items remain **unapproved** pending maintainer review.

## Pre-existing decisions (reconstructed from the code — not formally recorded before)
These describe why the project looks the way it does; confirm/revise with the maintainer.

- **Thin wrappers over raw GL, no engine framework.** Intentional for learning: the point
  is to see the GL calls, not hide them. → Keep wrappers thin; avoid over-abstraction.
- **GLFW + GLAD + GLM + stb**, vendored under `Libraries/source`. Standard, minimal modern
  OpenGL stack. Accepted.
- **OpenGL 3.3 core profile.** Broad compatibility; sufficient for the learning goals.
- **Immediate, hard-coded scene in `main.cpp`.** Acceptable at current scale; see Roadmap
  2.2 if object count grows.
- **Windows-only** (`Windows.h` for monitor metrics). Convenience, not a hard requirement;
  Roadmap 3.3 proposes GLFW-native alternatives if portability becomes a goal.

## Open questions for the maintainer
1. Is the **sphere** meant to be a plain light-source marker (position-only) or a fully
   lit/textured object? (Affects Roadmap 0.2 scope.)
2. Are **freetype/zlib** intended future features (text rendering) or leftovers to remove?
3. Is **model loading** (FBX/other) a real goal, or is `Cube.fbx` experimental?
4. Should the project stay Windows-only or move toward portability?
