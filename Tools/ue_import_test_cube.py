# Runs inside Unreal's Python. Imports Content/Kit/SM_Kit_TestCube.glb via Interchange
# and places one instance at the world origin.
import unreal
src = r"D:\ghost-in-the-stack-ue\GhostInTheStack\Content\Kit\SM_Kit_TestCube.glb"
dest = "/Game/Kit"
task = unreal.AssetImportTask()
task.filename = src
task.destination_path = dest
task.destination_name = "SM_Kit_TestCube"
task.replace_existing = True
task.automated = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
paths = list(task.imported_object_paths)
print("IMPORTED:", paths)
mesh = None
for p in paths:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        mesh = a
if mesh is None:
    mesh = unreal.load_asset(dest + "/SM_Kit_TestCube")
print("MESH:", mesh, "bounds:", mesh.get_bounds().box_extent if mesh else None)
if mesh:
    bs = unreal.EditorStaticMeshLibrary if hasattr(unreal, "EditorStaticMeshLibrary") else None
    try:
        print("collision prims:", unreal.StaticMeshEditorSubsystem().get_simple_collision_count(mesh) if hasattr(unreal, "StaticMeshEditorSubsystem") else "n/a")
    except Exception as e:
        print("collision query failed:", e)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == "SM_Kit_TestCube":
            eas.destroy_actor(a)
    actor = eas.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    actor.set_actor_label("SM_Kit_TestCube")
    print("PLACED:", actor.get_actor_label(), actor.get_actor_location())
