# Tools

Small scripts that drive the editor and Blender from outside. All of them run with the system
Python, not Unreal's.

| Script | What it does |
|---|---|
| `ue_mcp_client.py` | Minimal streamable-HTTP MCP client for the editor's server at `http://127.0.0.1:8000/mcp`. `python Tools/ue_mcp_client.py` initialises a session and lists the three meta-tools. Import it to call `list_toolsets`, `describe_toolset`, `call_tool`. |
| `ue_remote_python.py` | Runs a Python file inside the running editor via the Python plugin's remote execution (multicast `239.0.0.1:6766`, enabled in `DefaultEngine.ini`). `python Tools/ue_remote_python.py script.py`. Use this for anything the MCP toolsets can't do; the Programmatic toolset's sandbox cannot import `unreal`. |
| `blender_socket.py` | Talks to the Blender MCP addon socket on `127.0.0.1:9876`. `python Tools/blender_socket.py get_scene_info`, or `execute_code '{"file": "C:/path/script.py"}'` to run a script in Blender. |
| `blender_export_test_cube.py` | Phase 0 test: creates `SM_Kit_TestCube`, 1 m, origin at its base, and exports it as `.glb` to `Content/Kit/`. Run it in Blender through `blender_socket.py`. The seed of the kit export pipeline. |
| `ue_import_test_cube.py` | Phase 0 test: imports that `.glb` through Interchange and places it at the world origin. Run it through `ue_remote_python.py`. |

glTF import note: the `StaticMeshTools.import_file` MCP tool only accepts FBX and OBJ. glTF goes
through Interchange via `unreal.AssetImportTask`, which is why the import script exists. Interchange
nests the result under `<Name>/StaticMeshes/` and `<Name>/Materials/`; the Phase 0 pass moved the
assets to `Content/Kit/` root and added a box simple collision afterwards.

Origin note: in the template level `Lvl_FirstPerson` the world origin is enclosed by template
geometry — `SM_Cube13` (a 7 x 4 x 2 m block) with the yellow `SM_Cylinder` disc on top. A mesh
placed at (0,0,0) is inside that block and invisible until the block is hidden. The Phase 0
screenshot hid both temporarily (`set_is_temporarily_hidden_in_editor`) and restored them.
