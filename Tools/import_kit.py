# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/import_kit.py).
# Imports every Content/Kit/SM_Kit_*.glb through Interchange into /Game/Kit, flattens the
# per-file folders Interchange creates, makes sure each mesh has simple collision, and saves.
# Re-running re-imports in place, so actors that reference the meshes keep working.
import os
import unreal

KIT_DIR = os.path.join(unreal.Paths.project_content_dir(), "Kit")
DEST = "/Game/Kit"
EAL = unreal.EditorAssetLibrary
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)


def apply_sidecar_collision(mesh, sidecar):
    """Replaces the mesh's simple collision with the boxes the export wrote (Unreal space, cm)."""
    import json
    data = json.load(open(sidecar))
    SMS.remove_collisions(mesh)
    body = mesh.get_editor_property("body_setup")
    agg = body.get_editor_property("agg_geom")
    boxes = []
    for b in data.get("boxes", []):
        el = unreal.KBoxElem()
        cx, cy, cz = b["center"]; sx, sy, sz = b["size"]
        el.set_editor_property("center", unreal.Vector(cx, cy, cz))
        el.set_editor_property("x", sx); el.set_editor_property("y", sy); el.set_editor_property("z", sz)
        boxes.append(el)
    agg.set_editor_property("box_elems", boxes)
    body.set_editor_property("agg_geom", agg)
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
    mesh.set_editor_property("body_setup", body)
    body.invalidate_physics_data() if hasattr(body, "invalidate_physics_data") else None
    body.create_physics_meshes() if hasattr(body, "create_physics_meshes") else None
    mesh.modify()
    return SMS.get_simple_collision_count(mesh)


def import_one(glb_path):
    name = os.path.splitext(os.path.basename(glb_path))[0]
    task = unreal.AssetImportTask()
    task.filename = glb_path
    task.destination_path = DEST
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = list(task.imported_object_paths)

    # Interchange nests results under <name>/StaticMeshes and <name>/Materials; flatten them.
    mesh_path = f"{DEST}/{name}"
    moved = []
    for p in imported:
        obj_path = p.split(".")[0]
        base = os.path.basename(obj_path)
        target = f"{DEST}/{base}"
        if obj_path != target:
            if EAL.does_asset_exist(target):
                # The same material or texture came in with an earlier piece. Point this piece at
                # that one and drop the duplicate; deleting the existing asset would clear the
                # slots of every mesh already using it.
                EAL.consolidate_assets(EAL.load_asset(target), [EAL.load_asset(obj_path)])
            else:
                EAL.rename_asset(obj_path, target)
            moved.append((obj_path, target))
    sub = f"{DEST}/{name}"
    if EAL.does_directory_exist(sub) and not EAL.does_asset_exist(sub):
        for a in EAL.list_assets(sub, recursive=True):
            o = EAL.load_asset(a)
            if isinstance(o, unreal.ObjectRedirector):
                EAL.delete_asset(a)
        EAL.delete_directory(sub)

    mesh = EAL.load_asset(mesh_path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"{name}: no StaticMesh at {mesh_path}; imported={imported}")
    sidecar = os.path.splitext(glb_path)[0] + ".collision.json"
    collision = 0
    if os.path.isfile(sidecar):
        collision = apply_sidecar_collision(mesh, sidecar)
        note = "from sidecar"
    else:
        collision = SMS.get_simple_collision_count(mesh)
        if collision == 0:
            SMS.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
            collision = SMS.get_simple_collision_count(mesh)
            note = "box added"
        else:
            note = "kept"
    ext = mesh.get_bounds().box_extent
    for p in EAL.list_assets(DEST, recursive=False):
        EAL.save_asset(p.split(".")[0])
    print(f"{name}: extent=({ext.x:.0f},{ext.y:.0f},{ext.z:.0f}) cm  collision={collision} ({note})  moved={len(moved)}")
    return mesh_path


def fix_material_slots():
    """Binds every slot to the kit material of the slot's imported name.

    A re-import over an existing mesh keeps the old slot bindings, and a slot whose material
    was deleted falls back to WorldGridMaterial; both show up as the engine's grid.
    """
    fixed = 0
    for a in EAL.list_assets(DEST, recursive=False):
        o = EAL.load_asset(a.split(".")[0])
        if not isinstance(o, unreal.StaticMesh):
            continue
        mats = list(o.static_materials)
        changed = False
        for i, sm in enumerate(mats):
            # the slot name is the Blender material name on a fresh import; a stale slot keeps an
            # old name, and then the kit's rule applies: slot 0 is the trim sheet, slot 1 the glow
            want = str(sm.material_slot_name)
            if not EAL.does_asset_exist(f"{DEST}/{want}"):
                want = "M_Kit_Trim" if i == 0 else "M_Kit_Glow"
            current = sm.material_interface
            if current and current.get_name() == want:
                continue
            path = f"{DEST}/{want}"
            if EAL.does_asset_exist(path):
                sm.material_interface = EAL.load_asset(path)
                mats[i] = sm
                changed = True
        if changed:
            o.set_editor_property("static_materials", mats)
            EAL.save_loaded_asset(o, only_if_is_dirty=False)
            fixed += 1
    print(f"material slots fixed on {fixed} mesh(es)")
    # Interchange's instances point at the texture that came in beside them; if that texture was a
    # duplicate that got consolidated away, the parameter falls back to a white default. The kit's
    # rule: M_Kit_<Name> reads T_Kit_<Name> when that texture exists.
    MEL = unreal.MaterialEditingLibrary
    for a in EAL.list_assets(DEST, recursive=False):
        o = EAL.load_asset(a.split(".")[0])
        if not isinstance(o, unreal.MaterialInstanceConstant):
            continue
        tex_path = f"{DEST}/T_{o.get_name()[2:]}"
        if not EAL.does_asset_exist(tex_path):
            continue
        current = MEL.get_material_instance_texture_parameter_value(o, "BaseColorTexture")
        if not current or current.get_path_name() != EAL.load_asset(tex_path).get_path_name():
            MEL.set_material_instance_texture_parameter_value(o, "BaseColorTexture", EAL.load_asset(tex_path))
            MEL.update_material_instance(o)
            EAL.save_loaded_asset(o, only_if_is_dirty=False)
            print(f"{o.get_name()}: BaseColorTexture -> {tex_path}")


def main():
    files = sorted(f for f in os.listdir(KIT_DIR) if f.startswith("SM_Kit_") and f.lower().endswith(".glb"))
    if not files:
        print("no SM_Kit_*.glb in", KIT_DIR)
        return
    print(f"importing {len(files)} kit piece(s) from {KIT_DIR}")
    for f in files:
        import_one(os.path.join(KIT_DIR, f))
    fix_material_slots()
    print("kit assets:", EAL.list_assets(DEST, recursive=True))


main()
