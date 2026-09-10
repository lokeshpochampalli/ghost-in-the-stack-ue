# Ghost in the Stack (Unreal)

First-person 3D game that teaches introductory programming: read the previous maintainer's
scripts, predict what they do, run them, watch the station react, rewind, fix a line. Second
design cycle of the browser prototype, which remains the reference implementation for the
language spec, the decision record and the thirty golden trace fixtures.

Read `CLAUDE.md` first, then `docs/3D-REDESIGN.md` for the game and `PHASES-3D.md` for the
build order and status. `SETUP.md` is the machine setup, in order.

## Versions

| Component | Version |
|---|---|
| Unreal Engine | 5.8, Epic Games Launcher build, C++ First Person template |
| Compiler | Visual Studio Build Tools 2026, MSVC v14.44 x64 toolset, Windows SDK 10.0.28000 |
| Unreal MCP plugin | Epic's official `ModelContextProtocol` (ships with 5.8, experimental) plus `AllToolsets` |
| Claude Code plugin | `unreal-engine-skills-for-claude-code` 3.0.4, cloned to `D:\tools\ue-skills` |
| Blender | 5.2 with the Blender MCP addon |

## Build

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GhostInTheStackEditor Win64 Development -Project="D:\ghost-in-the-stack-ue\GhostInTheStack\GhostInTheStack.uproject" -WaitMutex
```

## MCP server

The editor starts the MCP server on launch; `bAutoStartServer=True` is committed in
`Config/DefaultEditorPerProjectUserSettings.ini`. It listens on `http://localhost:8000/mcp`,
and `.mcp.json` at the repo root points Claude Code at it under the name `unreal-mcp`.

Manual control from the editor console (`~`):

```
ModelContextProtocol.StartServer
ModelContextProtocol.StopServer
ModelContextProtocol.GenerateClientConfig ClaudeCode
```

Launch the editor with:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\ghost-in-the-stack-ue\GhostInTheStack\GhostInTheStack.uproject"
```

## Scripts

`Tools/` holds the out-of-editor helpers: an MCP client, a Python remote-execution runner, a
Blender socket client, and the Phase 0 cube export and import scripts. See `Tools/README.md`.

## Repo notes

- `Content/` is under Git LFS (`.uasset`, `.umap`, `.fbx`, `.glb`, `.gltf`, `.png`, `.wav`, `.blend`).
- `Fixtures/` holds the thirty golden traces as flat `name.py` and `name.trace.txt` pairs. Read-only.
- `docs/DECISIONS.md` is human-written and append-only. ADR-026 is the redesign decision; the next entry is ADR-027.
- Generated IDE files (`.vscode/`, `*.code-workspace`, `.ignore`) are gitignored; regenerate with
  `Build.bat -projectfiles ... -vscode` as described in `SETUP.md`.
- Derived-data and Zen caches are configured to `D:/UE_DDC` and `D:/UE_Zen` in `Config/DefaultEngine.ini`.
- Kit pipeline: `Content/Kit/Kit.blend` → `Tools/export_kit.py` → `Tools/import_kit.py`. Rules in `Tools/README.md`.
