# AGENTS.md

This repository's agent/AI guidance lives in **[CLAUDE.md](CLAUDE.md)** (read it first) and
the **[docs/](docs/)** folder. This file is a pointer so tools that look for `AGENTS.md`
find the same instructions — do not duplicate content here.

Quick map:
- **[CLAUDE.md](CLAUDE.md)** — what the project is, build & run, architecture summary,
  conventions (11-float vertex layout, texture units), editor controls, hard rules, status.
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** — current implementation, module table,
  editor interaction (picking/gizmos/focus/shortcuts/undo) and the area-light approximation.
- **[docs/DECISIONS.md](docs/DECISIONS.md)** — ADRs (ADR-0004 covers the scene editor).
- **[docs/ROADMAP.md](docs/ROADMAP.md)** — done phases and remaining work.
- **[docs/RENDERING_PIPELINE.md](docs/RENDERING_PIPELINE.md)** / **[docs/RESOURCE_MANAGEMENT.md](docs/RESOURCE_MANAGEMENT.md)**.

Build: `./build.ps1 -Run`. No test suite — validate by running the app and checking stderr
(the `Shader` class and `GLUtils.h glCheckError()` log GLSL/GL errors). No display in the
dev environment, so do not claim visual validation.
