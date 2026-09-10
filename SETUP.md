# SETUP.md

Everything that has to exist before Phase 0, in order. Cursor is the editor, Claude Code is the
driver, Unreal compiles from the command line. No Visual Studio IDE.

---

## 0. Hardware check

- **GPU.** Lumen needs DX12 / Shader Model 6 — any RTX or RX 6000+. On integrated graphics or a
  GTX 10-series, turn Lumen off (Project Settings → Rendering → Global Illumination → None) and
  use baked lighting. One setting, nothing else in the plan changes.
- **Disk.** 120 GB free. UE 5.7 ~60 GB, Build Tools ~10 GB, project grows.
- **RAM.** 32 GB comfortable, 16 GB workable with only the editor and Cursor open.
- **OS.** Windows 10/11.

---

## 1. Compiler — Build Tools for Visual Studio 2022

Unreal needs the MSVC compiler, not the Visual Studio IDE.

1. Microsoft's Visual Studio downloads page → scroll to **Tools for Visual Studio** → **Build
   Tools for Visual Studio 2022** → download and run.
2. Workload: tick **Desktop development with C++**.
3. Individual components tab, confirm ticked: **MSVC v143 – VS 2022 C++ x64/x86 build tools**,
   **Windows 11 SDK** (10 is fine too), **.NET 8.0 SDK**.
4. Install. ~10 GB.

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

## 3. Unreal Engine 5.7

1. Epic Games Launcher → Unreal Engine → Library → **+** → 5.7.x.
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

1. Epic Launcher → Launch UE 5.7 → **Games** → **First Person** → **C++** → Starter Content
   **off**, Raytracing **off** → name `GhostInTheStack`, location somewhere with the space, e.g.
   `D:\ghost-in-the-stack-ue`.
2. It will compile the template. Wait for the editor to open.
3. **Edit → Editor Preferences → General → Source Code → Source Code Editor → Visual Studio
   Code**. Then **File → Generate Visual Studio Code Project**. This writes
   `GhostInTheStack.code-workspace` and a `.vscode/` folder with build tasks and IntelliSense
   config.
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

Then `git add . && git commit -m "phase-0: UE 5.7 first-person C++ template"`.

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

Epic's official ModelContextProtocol plugin ships with 5.8 and is experimental; use the 5.7
backport (`thecodebrozilla/UE_MCP`).

1. In the project root: `git clone https://github.com/thecodebrozilla/UE_MCP Plugins/UE_MCP`
   — the repo root *is* a `Plugins/` directory.
2. Open the project. It asks to rebuild plugins → **Yes**. C++ compile, several minutes.
3. Edit → Plugins → confirm enabled: **ModelContextProtocol**, **ToolsetRegistry**,
   **AllToolsets**, **Python Editor Script Plugin**, **Editor Scripting Utilities**. Restart if
   asked.
4. Editor console (`~` key): `ModelContextProtocol.StartServer`
5. Then: `ModelContextProtocol.GenerateClientConfig ClaudeCode` — writes `.mcp.json` in the
   project root, which Claude Code reads.
6. Optional: set `bAutoStartServer=True` under
   `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]` in
   `Config/DefaultEngine.ini` so you never type step 4 again.

**Time-box: one hour.** If the backport won't compile by then, go to section 10 and use the
fallback as primary. Don't lose an evening to a community port.

---

## 9. Unreal MCP — the Claude Code side

Epic's skills plugin tells Claude Code how to use the tools.

1. Clone `EpicGames/unreal-engine-skills-for-claude-code-plugin` somewhere **outside** the project,
   e.g. `D:\tools\ue-skills`.
2. In Cursor's terminal, in the project root, run `claude`, then:
   ```
   /plugin marketplace add D:\tools\ue-skills
   /plugin install unreal-engine-skills-for-claude-code@ue-skills
   ```
   (The name after `@` is the folder name you cloned into.)
3. Verify: `/plugin` shows it enabled. `/mcp` shows `unreal-mcp` connected.
4. Ask: "List all actors in the current level." If it answers with the template actors, done.

The CLI and the extension share `~/.claude`, so the plugin is available in both.

---

## 10. Fallback, and Phase 2 addition — `remiphilippe/mcp-unreal`

A Go binary plus its own `MCPUnreal` editor plugin, targeting 5.7. Adds headless build and test
without the editor open, **viewport capture during play**, and **player control in PIE**. Phase 2
onward wants those for screenshots of the door opening.

Install per its README, then in Claude Code: `claude mcp add unreal-h -- mcp-unreal`. Runs
alongside section 8 without conflict. If section 8 failed, this is primary.

---

## 11. Blender

Confirm Blender MCP is connected from Cursor's Claude Code: "Create a 1 m cube in Blender and
report its dimensions." If it answers, fine. If not, reconnect it before Phase 0 — it's on the
Phase 0 acceptance list.

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

- Launch editor (server auto-starts if you did 8.6; otherwise `ModelContextProtocol.StartServer`).
- Open the `.code-workspace` in Cursor.
- `git status` clean; commit if not. MCP tools mutate live editor state and can delete assets
  in one call. A clean tree is the undo.
- Cursor terminal → `claude` → "go".
