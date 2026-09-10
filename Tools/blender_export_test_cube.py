import bpy, os
out_dir = r"D:\ghost-in-the-stack-ue\GhostInTheStack\Content\Kit"
name = "SM_Kit_TestCube"
# remove any previous copy
for o in list(bpy.data.objects):
    if o.name.startswith(name):
        bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.object.select_all(action='DESELECT')
# 1 m cube, origin at its base so it sits on the floor at Z=0
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.5))
cube = bpy.context.active_object
cube.name = name
cube.data.name = name
bpy.context.scene.cursor.location = (0, 0, 0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
mat = bpy.data.materials.get("M_Kit_Test") or bpy.data.materials.new("M_Kit_Test")
mat.use_nodes = True
mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.373, 0.541, 0.490, 1.0)  # oxidised copper #5F8A7D
cube.data.materials.clear(); cube.data.materials.append(mat)
bpy.ops.object.select_all(action='DESELECT')
cube.select_set(True); bpy.context.view_layer.objects.active = cube
os.makedirs(out_dir, exist_ok=True)
path = os.path.join(out_dir, name + ".glb")
bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_selection=True, export_apply=True, export_yup=True)
dims = tuple(round(v, 4) for v in cube.dimensions)
print("EXPORTED", path, os.path.getsize(path), "bytes; dims(m)=", dims, "loc=", tuple(cube.location))
