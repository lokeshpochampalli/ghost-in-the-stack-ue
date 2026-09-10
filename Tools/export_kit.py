"""Export the modular kit from Content/Kit/Kit.blend to one glTF binary per piece.

Run headless:
    blender --background Content/Kit/Kit.blend --python Tools/export_kit.py
or inside a running Blender that has Kit.blend open (Tools/blender_socket.py execute_code).

Pipeline rules (PHASES-3D.md, Phase 0):
  - One kit file, Kit.blend. 1 Blender unit = 1 cm (scene unit scale 0.01).
  - Every exportable piece is a mesh object named SM_Kit_<Name> in the "Kit" collection.
  - Collision is a mesh named UCX_SM_Kit_<Name> (convex), parented to the piece or in the
    "Kit_Collision" collection. Exported together so Unreal's importer picks it up; the Unreal
    import script also adds a box if no collision came through.
  - Output: Content/Kit/SM_Kit_<Name>.glb, Y-up, transforms applied, metres in the file
    (glTF is always metres; Blender's exporter ignores the scene unit scale, so the script
    scales by 0.01 on the way out). Unreal imports metres as centimetres, so 100 units here
    is 100 cm in the level.
"""
import bpy
import os
import sys

UNIT_TO_METRES = 0.01  # 1 Blender unit = 1 cm

def project_root():
    if bpy.data.filepath:
        return os.path.normpath(os.path.join(os.path.dirname(bpy.data.filepath), "..", ".."))
    return os.getcwd()

def kit_objects():
    kit = bpy.data.collections.get("Kit")
    objs = list(kit.all_objects) if kit else list(bpy.data.objects)
    return [o for o in objs if o.type == 'MESH' and o.name.startswith("SM_Kit_")]

def collision_for(piece):
    name = "UCX_" + piece.name
    hits = [o for o in bpy.data.objects if o.type == 'MESH' and (o.name == name or o.name.startswith(name + "."))]
    hits += [c for c in piece.children if c.type == 'MESH' and c.name.startswith("UCX_") and c not in hits]
    return hits

def check_units():
    u = bpy.context.scene.unit_settings
    if abs(u.scale_length - UNIT_TO_METRES) > 1e-9:
        raise SystemExit(f"Kit.blend unit scale is {u.scale_length}, expected {UNIT_TO_METRES} (1 unit = 1 cm)")

def export_piece(piece, out_dir):
    colls = collision_for(piece)
    if not colls:
        print(f"  WARNING {piece.name}: no UCX_ collision mesh; Unreal import will add a box")
    # Work on temporary copies scaled to metres so the glTF comes out at the right size.
    copies = []
    for src in [piece] + colls:
        dup = src.copy(); dup.data = src.data.copy(); dup.name = "__export__" + src.name
        bpy.context.scene.collection.objects.link(dup)
        dup.parent = None
        dup.matrix_world = src.matrix_world
        copies.append((src, dup))
    bpy.ops.object.select_all(action='DESELECT')
    for src, dup in copies:
        dup.select_set(True)
    bpy.context.view_layer.objects.active = copies[0][1]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for src, dup in copies:
        dup.scale = (UNIT_TO_METRES,) * 3
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for src, dup in copies:
        # exporter writes object and mesh names; hand the originals' names to the copies
        src.name = "__hold__" + src.name
        src.data.name = "__hold__" + src.data.name
        dup.name = src.name.replace("__hold__", "")
        dup.data.name = src.data.name.replace("__hold__", "")
    out = os.path.join(out_dir, piece.name.replace("__hold__", "") + ".glb")
    try:
        bpy.ops.export_scene.gltf(
            filepath=out, export_format='GLB', use_selection=True,
            export_apply=True, export_yup=True, export_materials='EXPORT',
            export_animations=False, export_skins=False, export_cameras=False, export_lights=False)
    finally:
        for src, dup in copies:
            bpy.data.meshes.remove(dup.data)
            src.name = src.name.replace("__hold__", "")
            src.data.name = src.data.name.replace("__hold__", "")
    size = os.path.getsize(out)
    print(f"  exported {os.path.basename(out)} ({size} bytes) collision={len(colls)} dims_cm={tuple(round(v, 2) for v in piece.dimensions)}")
    return out

def main():
    check_units()
    out_dir = os.path.join(project_root(), "Content", "Kit")
    os.makedirs(out_dir, exist_ok=True)
    pieces = kit_objects()
    if not pieces:
        raise SystemExit("no SM_Kit_* mesh objects found in the Kit collection")
    print(f"Exporting {len(pieces)} kit piece(s) to {out_dir}")
    for p in pieces:
        export_piece(p, out_dir)
    print("done")

if __name__ == "__main__":
    main()
