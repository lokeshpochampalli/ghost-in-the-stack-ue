# Tools

Small scripts that drive the editor and Blender from outside. All of them run with the system
Python, not Unreal's.

| Script | What it does |
|---|---|
| `ue_mcp_client.py` | Minimal streamable-HTTP MCP client for the editor's server at `http://127.0.0.1:8000/mcp`. `python Tools/ue_mcp_client.py` initialises a session and lists the three meta-tools. Import it to call `list_toolsets`, `describe_toolset`, `call_tool`. |
| `ue_remote_python.py` | Runs a Python file inside the running editor via the Python plugin's remote execution (multicast `239.0.0.1:6766`, enabled in `DefaultEngine.ini`). `python Tools/ue_remote_python.py script.py`. Use this for anything the MCP toolsets can't do; the Programmatic toolset's sandbox cannot import `unreal`. |
| `blender_socket.py` | Talks to the Blender MCP addon socket on `127.0.0.1:9876`. `python Tools/blender_socket.py get_scene_info`, or `execute_code '{"file": "C:/path/script.py"}'` to run a script in Blender. |
| `export_kit.py` | The kit export pipeline. Reads `Content/Kit/Kit.blend`, exports every `SM_Kit_<Name>` mesh in the `Kit` collection to `Content/Kit/SM_Kit_<Name>.glb` and its `UCX_SM_Kit_<Name>` collision as a `SM_Kit_<Name>.collision.json` sidecar (Unreal's glTF importer drops UCX meshes), transforms applied, Y-up. Run headless from the project root: `"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background Content/Kit/Kit.blend --python Tools/export_kit.py` |
| `setup_sector1_content.py` | Builds the Sector 1 content in the editor: creates and maps `IA_Interact` (E) and `IA_Rewind` (hold R) and binds them on the player Blueprint, creates the script assets under `/Game/Scripts` (the airlock door with Ilse's bug, the corridor lights) with VANT's predictions, hints, intro, outro, goal and run costs, and rebuilds `/Game/Sectors/L_Sector1_Airlock` (corridor, door, two terminals, wall display, lights, power gauge, generator, emergency light, exposure). Re-runnable; run through `ue_remote_python.py`. |
| `run_interpreter_tests.cmd` | Runs the `GhostInTheStack.Interpreter` automation tests headlessly via `UnrealEditor-Cmd` and prints pass/fail counts from `Saved/Logs/GitsTests.log`. |
| `import_kit.py` | The Unreal side of the pipeline. Imports every `Content/Kit/SM_Kit_*.glb` through Interchange into `/Game/Kit`, flattens Interchange's per-file folders, applies the boxes from each `.collision.json` sidecar (a single bounds box if there is none), saves. Re-import in place, so placed actors keep their mesh. Run through `ue_remote_python.py`. |

## Kit pipeline rules

- **One kit file:** `Content/Kit/Kit.blend`. Scene units are centimetres (unit scale 0.01), so
  1 Blender unit = 1 cm and a 100-unit cube is 1 m in Unreal. `export_kit.py` refuses to run if
  the unit scale is anything else.
- **Naming:** a kit piece is a mesh object `SM_Kit_<Name>` in the `Kit` collection. Its collision
  is a convex mesh `UCX_SM_Kit_<Name>`, parented to the piece (the `Kit_Collision` collection
  holds them). Pieces without a UCX mesh get a box collision on import and a warning on export.
- **Collision travels as a sidecar.** Interchange's glTF import discards `UCX_` meshes, so
  `export_kit.py` writes the UCX meshes' bounds (in Unreal space: x, -y, z, cm) to
  `SM_Kit_<Name>.collision.json` and `import_kit.py` adds them as box elements on the body setup.
  Box-shaped UCX pieces are exact; anything else becomes its bounding box.
- **Origin:** at the piece's base, on the floor, so placing at Z=0 stands it on the ground.
- **Export:** one `.glb` per piece. glTF files are always metres and Blender's exporter ignores
  the scene unit scale, so the script exports temporary copies scaled by 0.01. Unreal's glTF
  importer turns metres back into centimetres.
- **Import:** `import_kit.py`, because the `StaticMeshTools.import_file` MCP tool only accepts FBX
  and OBJ, and the Programmatic toolset's sandbox cannot import `unreal`. glTF goes through
  Interchange via `unreal.AssetImportTask`.
- Fix the pipeline, not individual assets.

Origin note: in the template level `Lvl_FirstPerson` the world origin is enclosed by template
geometry — `SM_Cube13` (a 7 x 4 x 2 m block) with the yellow `SM_Cylinder` disc on top. A mesh
placed at (0,0,0) is inside that block and invisible until the block is hidden. The Phase 0
screenshot hid both temporarily (`set_is_temporarily_hidden_in_editor`) and restored them.
