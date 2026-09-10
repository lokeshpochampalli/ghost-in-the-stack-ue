# SETUP.md

Everything that has to exist before Phase 0, in order. Cursor is the editor, Claude Code is the
driver, Unreal compiles from the command line. No Visual Studio IDE.

---

## 0. Hardware check

- **GPU.** Lumen needs DX12 / Shader Model 6 — any RTX or RX 6000+. On integrated graphics or a
  GTX 10-series, turn Lumen off (Project Settings → Rendering → Global Illumination → None) and
  use baked lighting. One setting, nothing else in the plan changes.
- **Disk.** 120 GB free. UE 5.8 ~60 GB, Build Tools ~10 GB, project grows.
- **RAM.** 32 GB comfortable, 16 GB workable with only the editor and Cursor open.
- **OS.** Windows 10/11.

---

## 1. Compiler — Build Tools for Visual Studio 2026 (2022 also works)

Unreal needs the MSVC compiler, not the Visual Studio IDE.

1. Microsoft's Visual Studio downloads page → scroll to **Tools for Visual Studio** → **Build
   Tools for Visual Studio** → download and run.
2. Workload: tick **Desktop development with C++**.
3. Individual components tab, confirm ticked: **MSVC v143 – VC++ 2022 C++ x64/x86 build tools
   (v14.44-17.14)** — the x64/x86 entry, not the ARM one; UE 5.8 picks the 14.44 toolset —
   **Windows 11 SDK** (10.0.28000 verified), **.NET Framework 4.8 SDK**, **.NET 8.0 runtime**.
4. Install. ~10 GB.

If only the ARM variant of the toolset is ticked, the template compiles but linking fails with
`LNK1181: cannot open input file 'delayimp.lib'`. Tick the x64/x86 component and rebuild.

Must be done **before** step 5. The C++ project template refuses to generate without a compiler.

If UnrealBuildTool later complains about something Build Tools lacks, install Visual Studio
Community with the same workload and never open it. Same toolchain.

---

## 2. Git, Git LFS, Git Bash

1. Install **Git for Windows**. During setup choose **"Git from the command line and also from
   3rd-party software"** — this puts `bash` on PATH, which the Epic Claude Code plugin needs.
2. `git lfs install` once in any terminal.
3. Verify in PowerShell: `git --version`, `git lfs --version`, `bash --version`. All three must
   answer.

---

## 3. Unreal Engine 5.8

1. Epic Games Launcher → Unreal Engine → Library → **+** → 5.8.x.
2. Options: untick **Editor symbols for debugging** (saves ~20 GB). Target platforms: Windows only.
3. Install. Go do something else; it's an hour.

---

## 4. Cursor

You have it. Configure it:

1. **Claude Code extension.** Anthropic's Claude Code install page → **Install for Cursor**. Or
   in Cursor: Extensions → search "Claude Code" (it's on Open VSX) → install.
2. **Default terminal = Git Bash.** Settings → search "terminal integrated default profile
   windows" → set to **Git Bash**. This is what makes the Epic plugin's session hook run.
3. **C/C++ extension** (Microsoft, or the Open VSX equivalent) for IntelliSense. Optional —
   Claude Code doesn't use it, but you'll want it when reading generated code.
4. Open Cursor's integrated terminal and run `claude --version`. If the CLI isn't installed,
   install it per Anthropic's docs, then confirm.

Rule: Cursor's own AI and Claude Code both edit this repo fine, but never the same task at once.
Claude Code holds the MCP connection to Unreal and is the driver for every phase.

---

## 5. Create the project

1. Epic Launcher → Launch UE 5.8 → **Games** → **First Person** → **C++** → Starter Content
   **off**, Raytracing **off** → name `GhostInTheStack`, location somewhere with the space, e.g.
   `D:\ghost-in-the-stack-ue`.
2. It will compile the template. Wait for the editor to open.
3. Generate the VS Code project files from the command line (no editor needed):
   ```
   "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="D:\ghost-in-the-stack-ue\GhostInTheStack\GhostInTheStack.uproject" -game -engine -vscode
   ```
   This writes `GhostInTheStack.code-workspace` (build tasks and launch configs live inside it)
   and a `.vscode/` folder with IntelliSense config. Both contain absolute machine paths and are
   gitignored, as is the generated `.ignore`; that file tells ripgrep to skip `docs/` and
   `Fixtures/`, so delete those two lines after regenerating or Claude Code's search will not
   see the spec.
4. Open `GhostInTheStack.code-workspace` in Cursor. Unreal won't auto-launch Cursor because it
   doesn't recognise it as VS Code; you open the workspace yourself. That's the only difference
   from the VS Code flow.

---

## 6. Git the project

In Cursor's terminal, in the project root:

```bash
git init
git lfs track "*.uasset" "*.umap" "*.fbx" "*.glb" "*.gltf" "*.png" "*.wav" "*.blend"
```

Create `.gitignore`:

```
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
*.sln
*.VC.db
*.opensdf
*.opendb
*.sdf
*.suo
```

Then `git add . && git commit -m "phase-0: UE 5.8 first-person C++ template"`.

---

## 7. Bring over the reference material

From the TypeScript repo, copy into the new one and commit:

```
docs/02-LANGUAGE-SPEC.md   → docs/LANGUAGE-SPEC.md
docs/DECISIONS.md          → docs/DECISIONS.md     (continue from ADR-026)
tests/fixtures/            → Fixtures/              (all 30, source.py + expected trace)
```

`docs/3D-REDESIGN.md`, `CLAUDE.md`, `PHASES-3D.md` and this file are already in this folder;
put them at the project root. Nothing else comes over. The interpreter is ported from the
fixtures and the spec, not from the TypeScript. Commit.

---

## 8. Unreal MCP — the Unreal side

Epic's official `ModelContextProtocol` plugin ships prebuilt with the 5.8 launcher build under
`Engine/Plugins/Experimental`, alongside `ToolsetRegistry` and the `Toolsets/` family. No clone,
no plugin compile. All of this is already committed in this repo; the steps are recorded so it can
be redone on a fresh machine.

1. In `GhostInTheStack.uproject`, the `Plugins` array enables **ModelContextProtocol** (with
   `TargetAllowList: ["Editor"]` so the server never ships in a packaged game),
   **ToolsetRegistry**, **AllToolsets**, **PythonScriptPlugin**, **EditorScriptingUtilities**.
   `AllToolsets` is the aggregator that pulls in the 21 toolset plugins; without it the server
   starts but exposes no tools.
2. Rebuild from the command line after any `.uproject` change:
   ```
   "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GhostInTheStackEditor Win64 Development -Project="D:\ghost-in-the-stack-ue\GhostInTheStack\GhostInTheStack.uproject" -WaitMutex
   ```
3. Auto-start is on via `Config/DefaultEditorPerProjectUserSettings.ini`:
   ```ini
   [/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
   bAutoStartServer=True
   ```
   It has to be that file, not `DefaultEngine.ini`: the settings class is declared
   `config=EditorPerProjectUserSettings`, so the engine ini is never consulted for it.
4. Manual control from the editor console (`~` key): `ModelContextProtocol.StartServer`,
   `ModelContextProtocol.StopServer`, `ModelContextProtocol.RefreshTools`.
5. `ModelContextProtocol.GenerateClientConfig ClaudeCode` rewrites `.mcp.json` from the current
   port and path. The committed one is hand-written to the same shape: HTTP transport,
   `http://localhost:8000/mcp`, server name `unreal-mcp`.

Fallback, only if the official plugin breaks in a future engine update: `thecodebrozilla/UE_MCP`
cloned into `Plugins/UE_MCP`. Time-box it to an hour.

**Caches live on D:.** `Config/DefaultEngine.ini` points the local derived-data cache at
`D:/UE_DDC` (`[DerivedDataCacheStores] InstalledLocal=(Base=Local, Path="D:/UE_DDC")`) and the
Zen local store at `D:/UE_Zen` (`[Zen.AutoLaunch] DataPath=D:/UE_Zen`). The default location on
C: ran out of space and the Zen cache started refusing writes with HTTP 507, which shows up as
repeated shader and mesh builds. Keep at least 5 GB free on C: regardless; the editor and the
launcher still write logs, crash data and the Zen install there.

**Python inside the editor.** The MCP toolsets cover most editing, but the Programmatic toolset's
sandbox cannot import `unreal`, and some jobs (glTF import through Interchange, saving the level)
need real editor Python. `bRemoteExecution=True` is committed under
`[/Script/PythonScriptPlugin.PythonScriptPluginSettings]` in `Config/DefaultEngine.ini`, so the
editor listens on multicast `239.0.0.1:6766` (bound to `127.0.0.1`). `Tools/ue_remote_python.py`
runs a script file there using Epic's own `remote_execution.py` client. See `Tools/README.md`.

---

## 9. Unreal MCP — the Claude Code side

Epic's skills plugin tells Claude Code how to use the tools.

1. Clone `EpicGames/unreal-engine-skills-for-claude-code-plugin` somewhere **outside** the project,
   to `D:\tools\ue-skills` exactly — the committed `.claude/settings.json` points there.
2. `.claude/settings.json` (committed) registers that directory as a marketplace named
   `ue-skills` under `extraKnownMarketplaces` and enables
   `unreal-engine-skills-for-claude-code@ue-skills`. Anyone who trusts the project folder is
   prompted to install it. If that ever fails, the plugin is also on Anthropic's official
   marketplace: `/plugin install unreal-engine-skills-for-claude-code@claude-plugins-official`.
3. Verify: `/plugin` shows it enabled. `/mcp` shows `unreal-mcp` connected. The project-scoped
   `.mcp.json` server needs a one-time approval the first time `claude` runs in this folder.
4. Ask: "List all actors in the current level." If it answers with the template actors, done.

The CLI and the extension share `~/.claude`, so the plugin is available in both.

---

## 10. Fallback, and Phase 2 addition — `remiphilippe/mcp-unreal`

A Go binary plus its own `MCPUnreal` editor plugin, targeting 5.7 at the time of writing; check
5.8 support before relying on it. Adds headless build and test
without the editor open, **viewport capture during play**, and **player control in PIE**. Phase 2
onward wants those for screenshots of the door opening.

Install per its README, then in Claude Code: `claude mcp add unreal-h -- mcp-unreal`. Runs
alongside section 8 without conflict. If section 8 failed, this is primary.

---

## 11. Blender

Confirm Blender MCP is connected from Cursor's Claude Code: "Create a 1 m cube in Blender and
report its dimensions." If it answers, fine. If not, reconnect it before Phase 0 — it's on the
Phase 0 acceptance list.

Two halves: the Blender addon (Edit → Preferences → Add-ons → Blender MCP → **Connect to MCP
server**) listens on `127.0.0.1:9876`, and the `blender-mcp` server (`uvx blender-mcp`, stdio)
bridges it to Claude Code. The server must be registered for **this project's** scope or
globally; a registration made from another folder is invisible here. With the addon connected,
any script can also drive it directly over the socket.

The kit pipeline itself does not need the MCP: `Content/Kit/Kit.blend` is the one kit file and
`Tools/export_kit.py` runs headless (`blender --background Content/Kit/Kit.blend --python
Tools/export_kit.py`), then `Tools/import_kit.py` brings the `.glb` files into `/Game/Kit`.
Rules in `Tools/README.md`.

---

## 12. Smoke test

Editor open, MCP server started, Claude Code running in Cursor's terminal:

1. "List all actors in the current level."
2. "Take a screenshot of the viewport."
3. "Create a cube in Blender, export it as glTF to `Content/Kit/`, import it, and place it at
   the origin."

Three passes → say **"go"**.

---

## Daily

- Launch editor; the MCP server auto-starts (setting committed in
  `Config/DefaultEditorPerProjectUserSettings.ini`). Otherwise `ModelContextProtocol.StartServer`.
- Open the `.code-workspace` in Cursor.
- `git status` clean; commit if not. MCP tools mutate live editor state and can delete assets
  in one call. A clean tree is the undo.
- Cursor terminal → `claude` → "go".
