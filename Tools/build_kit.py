"""Builds the modular kit in Kit.blend from primitives, with one trim sheet (Phase 6).

Run headless from the project root (after Tools/make_trim_sheet.py):
    blender --background Content/Kit/Kit.blend --python Tools/build_kit.py

Every piece is a mesh SM_Kit_<Name> in the "Kit" collection, origin at its base on the floor,
built from boxes whose faces are UV-mapped onto a band of Content/Kit/T_Kit_Trim.png (the long
axis tiles along U, one repeat per metre; the short axis spans the band). A bevel modifier
gives the chunky chamfered edges of cast and folded metal; the exporter applies it. Collision
is one UCX_ box per part, exported as the sidecar the importer reads. Units: 1 unit = 1 cm.

Style: cold, over-engineered, 1978. Low-poly, strong silhouettes, the art budget on lighting.
"""
import bpy
import bmesh
import os
from mathutils import Vector

scene = bpy.context.scene
assert abs(scene.unit_settings.scale_length - 0.01) < 1e-9, "Kit.blend must be in centimetres"
ROOT = os.path.normpath(os.path.join(os.path.dirname(bpy.data.filepath), "..", ".."))
TRIM = os.path.join(ROOT, "Content", "Kit", "T_Kit_Trim.png")
TILE_CM = 100.0
BAND_V = {  # band name -> (v0, v1) in glTF/Blender UV space (v up: band 0 is at the top)
    name: (1.0 - (i + 1) / 8.0, 1.0 - i / 8.0)
    for i, name in enumerate(["bone", "slate", "copper", "ink", "hazard", "grating", "ceiling", "signage"])
}
INSET = 0.004  # keep UVs off the band edges so filtering never bleeds a neighbour in

kit = bpy.data.collections.get("Kit") or bpy.data.collections.new("Kit")
if kit.name not in [c.name for c in scene.collection.children]:
    scene.collection.children.link(kit)
coll = bpy.data.collections.get("Kit_Collision") or bpy.data.collections.new("Kit_Collision")
if coll.name not in [c.name for c in scene.collection.children]:
    scene.collection.children.link(coll)


# --- materials -------------------------------------------------------------------------------
def trim_material():
    m = bpy.data.materials.get("M_Kit_Trim") or bpy.data.materials.new("M_Kit_Trim")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    tex = nt.nodes.new("ShaderNodeTexImage")
    img = bpy.data.images.get("T_Kit_Trim") or bpy.data.images.load(TRIM)
    img.name = "T_Kit_Trim"
    img.reload()
    tex.image = img
    tex.interpolation = 'Closest' if False else 'Linear'
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.62
    bsdf.inputs["Metallic"].default_value = 0.0
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m


def glow_material():
    m = bpy.data.materials.get("M_Kit_Glow") or bpy.data.materials.new("M_Kit_Glow")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (0.95, 0.88, 0.72, 1.0)
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.86, 0.62, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 3.0
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m


MAT = trim_material()
GLOW = glow_material()


# --- geometry --------------------------------------------------------------------------------
def remove(name):
    for o in list(bpy.data.objects):
        if o.name == name or o.name.startswith(name + ".") or o.name.startswith("UCX_" + name):
            bpy.data.objects.remove(o, do_unlink=True)


def add_box(bm, uv_layer, size, center, band, tile=TILE_CM, across_axis=None):
    """A box in the bmesh with each face mapped onto `band`: long axis along U, short across V.

    Centres are given in Unreal handedness (the exporter maps Blender +Y to Unreal -Y), so a
    piece described with its doorway on +Y has its doorway on +Y in the level.
    """
    sx, sy, sz = size
    cx, cy, cz = center
    cy = -cy
    hx, hy, hz = sx / 2, sy / 2, sz / 2
    v = [bm.verts.new((cx + dx * hx, cy + dy * hy, cz + dz * hz))
         for dx in (-1, 1) for dy in (-1, 1) for dz in (-1, 1)]
    # index: dx*4 + dy*2 + dz with (-1 -> 0, 1 -> 1)
    def V(dx, dy, dz):
        return v[(dx > 0) * 4 + (dy > 0) * 2 + (dz > 0)]
    faces = [
        ((V(-1, -1, -1), V(-1, 1, -1), V(-1, 1, 1), V(-1, -1, 1)), 'x'),   # -x
        ((V(1, -1, -1), V(1, -1, 1), V(1, 1, 1), V(1, 1, -1)), 'x'),       # +x
        ((V(-1, -1, -1), V(-1, -1, 1), V(1, -1, 1), V(1, -1, -1)), 'y'),   # -y
        ((V(-1, 1, -1), V(1, 1, -1), V(1, 1, 1), V(-1, 1, 1)), 'y'),       # +y
        ((V(-1, -1, -1), V(1, -1, -1), V(1, 1, -1), V(-1, 1, -1)), 'z'),   # -z
        ((V(-1, -1, 1), V(-1, 1, 1), V(1, 1, 1), V(1, -1, 1)), 'z'),       # +z
    ]
    v0, v1 = BAND_V[band]
    v0 += INSET; v1 -= INSET
    for verts, normal_axis in faces:
        try:
            f = bm.faces.new(verts)
        except ValueError:
            continue
        axes = [a for a in 'xyz' if a != normal_axis]
        lengths = {'x': sx, 'y': sy, 'z': sz}
        # the axis that tiles along U: the longer one, unless told otherwise
        if across_axis and across_axis in axes:
            across = across_axis
        else:
            across = min(axes, key=lambda a: lengths[a])
            if normal_axis != 'z' and 'z' in axes:
                across = 'z'  # walls: height spans the band, length tiles along it
        along = [a for a in axes if a != across][0]
        idx = {'x': 0, 'y': 1, 'z': 2}
        mirrored = (cx, cy, cz)
        lo_across = mirrored[idx[across]] - lengths[across] / 2
        for loop in f.loops:
            co = loop.vert.co
            u = co[idx[along]] / tile
            t = (co[idx[across]] - lo_across) / max(lengths[across], 1e-6)
            loop[uv_layer].uv = (u, v0 + t * (v1 - v0))
    return faces


def make_piece(name, parts, bevel=1.2, glow_parts=()):
    """parts: list of (size, center, band). Returns the mesh object with a UCX box per part."""
    remove(name)
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new("UVMap")
    mat_index = []
    for size, center, band in parts:
        add_box(bm, uv_layer, size, center, band)
        mat_index.append(0)
    for gsize, gcenter in glow_parts:
        add_box(bm, uv_layer, gsize, gcenter, "bone")
    bm.to_mesh(me)
    bm.free()
    me.materials.append(MAT)
    me.materials.append(GLOW)
    # glow faces are the last ones added
    n_glow_faces = 6 * len(glow_parts)
    if n_glow_faces:
        for p in me.polygons[-n_glow_faces:]:
            p.material_index = 1
    obj = bpy.data.objects.new(name, me)
    kit.objects.link(obj)
    if bevel > 0:
        b = obj.modifiers.new("Bevel", 'BEVEL')
        b.width = bevel
        b.segments = 2
        b.limit_method = 'ANGLE'
        b.angle_limit = 0.6
        b.harden_normals = False
    for i, (size, center, band) in enumerate(list(parts) + [(g[0], g[1], "bone") for g in glow_parts]):
        cme = bpy.data.meshes.new("UCX")
        cbm = bmesh.new()
        cuv = cbm.loops.layers.uv.new("UVMap")
        add_box(cbm, cuv, size, center, "ink")
        cbm.to_mesh(cme)
        cbm.free()
        c = bpy.data.objects.new(f"UCX_{name}" + ("" if i == 0 else f"_{i:02d}"), cme)
        coll.objects.link(c)
        c.parent = obj
        c.display_type = 'WIRE'
        c.hide_render = True
    return obj


# --- the pieces ------------------------------------------------------------------------------
L, W, H, T = 400, 300, 300, 20   # corridor length, width, height, shell thickness
DADO = 90                        # slate dado height on corridor walls
DW, DH, FT = 120, 220, 30        # doorway width, height, frame thickness


def corridor_walls(x0, length, side, opening=None):
    """Wall parts along x for one side (+1 / -1 in y). `opening` = (x_centre, width) leaves a doorway."""
    y = side * (W / 2 + T / 2)
    parts = []
    segments = [(x0, x0 + length)]
    if opening:
        ox, ow = opening
        segments = [(x0, ox - ow / 2), (ox + ow / 2, x0 + length)]
    for a, b in segments:
        if b - a <= 0:
            continue
        cx, ln = (a + b) / 2, b - a
        parts.append(((ln, T, DADO), (cx, y, DADO / 2), "slate"))
        parts.append(((ln, T + 2, 6), (cx, y - side * 1, DADO + 3), "copper"))
        parts.append(((ln, T, H - DADO - 6), (cx, y, DADO + 6 + (H - DADO - 6) / 2), "bone"))
    if opening:
        ox, ow = opening
        # the lintel over the doorway
        parts.append(((ow, T, H - DH), (ox, y, DH + (H - DH) / 2), "bone"))
        parts.append(((ow + 8, T + 2, 8), (ox, y - side * 1, DH + 4), "copper"))
    return parts


def corridor(name, opening_side=None):
    parts = [((L, W, T), (L / 2, 0, -T / 2), "grating"),
             ((L, W, T), (L / 2, 0, H + T / 2), "ceiling")]
    parts += corridor_walls(0, L, -1, opening=(L / 2, DW) if opening_side == -1 else None)
    parts += corridor_walls(0, L, +1, opening=(L / 2, DW) if opening_side == +1 else None)
    return make_piece(name, parts, bevel=0.8)


corridor("SM_Kit_CorridorSegment")
corridor("SM_Kit_CorridorDoorway", opening_side=+1)

# corner: occupies x 0..300, y -150..150; open at x=0 (entry) and y=+150 (exit); walls on +x and -y
CL = W
parts = [((CL, W, T), (CL / 2, 0, -T / 2), "grating"),
         ((CL, W, T), (CL / 2, 0, H + T / 2), "ceiling")]
# far wall at +x, spanning the full width, built like a corridor wall but along y
parts += [((T, W + 2 * T, DADO), (CL + T / 2, 0, DADO / 2), "slate"),
          ((T + 2, W + 2 * T, 6), (CL + T / 2 - 1, 0, DADO + 3), "copper"),
          ((T, W + 2 * T, H - DADO - 6), (CL + T / 2, 0, DADO + 6 + (H - DADO - 6) / 2), "bone")]
parts += corridor_walls(0, CL, -1)
make_piece("SM_Kit_CorridorCorner", parts, bevel=0.8)

# wall cap: closes an open corridor end; a slab with the dado and copper line
parts = [((T, W + 2 * T, DADO), (T / 2, 0, DADO / 2), "slate"),
         ((T + 2, W + 2 * T, 6), (T / 2 - 1, 0, DADO + 3), "copper"),
         ((T, W + 2 * T, H - DADO - 6), (T / 2, 0, DADO + 6 + (H - DADO - 6) / 2), "bone")]
make_piece("SM_Kit_WallCap", parts, bevel=0.8)

# door frame: a wall slab across the corridor with a 120 x 220 opening, hazard sill, copper jambs
side_w = (W + 2 * T - DW) / 2
parts = [((FT, side_w, H), (0, -(DW / 2 + side_w / 2), H / 2), "slate"),
         ((FT, side_w, H), (0, (DW / 2 + side_w / 2), H / 2), "slate"),
         ((FT, DW, H - DH), (0, 0, DH + (H - DH) / 2), "slate"),
         ((FT + 4, 8, DH), (0, -(DW / 2 + 4), DH / 2), "copper"),
         ((FT + 4, 8, DH), (0, (DW / 2 + 4), DH / 2), "copper"),
         ((FT + 4, DW + 16, 8), (0, 0, DH + 4), "copper"),
         ((FT + 6, DW + 40, 4), (0, 0, 2), "hazard")]
make_piece("SM_Kit_DoorFrame", parts, bevel=1.0)

# door panel: copper, a recessed ink window, an ink kick plate; origin at its base centre
parts = [((10, DW - 4, DH - 2), (0, 0, (DH - 2) / 2), "copper"),
         ((12, 40, 30), (0, 0, 150), "ink"),
         ((12, DW - 12, 30), (0, 0, 20), "ink")]
make_piece("SM_Kit_DoorPanel", parts, bevel=1.0)

# terminal: ink pedestal, slate head with the screen face at x=-20 (z 95..140), copper edge
parts = [((40, 60, 95), (0, 0, 47.5), "ink"),
         ((30, 70, 45), (-5, 0, 117.5), "slate"),
         ((4, 74, 4), (-20, 0, 141), "copper"),
         ((4, 74, 4), (-20, 0, 94), "copper"),
         ((44, 64, 6), (0, 0, 3), "hazard")]
make_piece("SM_Kit_Terminal", parts, bevel=1.0)

# wall display: ink panel with a copper bezel; face at x=-6; origin bottom-centre of the back
parts = [((6, 120, 70), (-3, 0, 35), "ink"),
         ((2, 124, 4), (-6, 0, 2), "copper"),
         ((2, 124, 4), (-6, 0, 68), "copper"),
         ((2, 4, 70), (-6, -61, 35), "copper"),
         ((2, 4, 70), (-6, 61, 35), "copper")]
make_piece("SM_Kit_WallDisplay", parts, bevel=0.6)

# wall gauge: the same panel language at the same size (the level places it at half scale),
# with a slate housing and a bone enamel face rim
parts = [((6, 120, 70), (-3, 0, 35), "slate"),
         ((2, 126, 6), (-6, 0, 3), "bone"),
         ((2, 126, 6), (-6, 0, 67), "bone"),
         ((2, 6, 70), (-6, -62, 35), "bone"),
         ((2, 6, 70), (-6, 62, 35), "bone")]
make_piece("SM_Kit_WallGauge", parts, bevel=0.6)

# cold store unit: a chest with a bone lid, slate body, ink frost window, copper handle
parts = [((70, 130, 90), (0, 0, 45), "slate"),
         ((74, 134, 14), (0, 0, 97), "bone"),
         ((4, 60, 24), (-36, 0, 60), "ink"),
         ((6, 40, 4), (-38, 0, 84), "copper"),
         ((74, 134, 6), (0, 0, 3), "hazard")]
make_piece("SM_Kit_ColdStoreUnit", parts, bevel=1.2)

# ceiling light: slate housing hanging from its mount point (origin at the top), bone diffuser that glows
parts = [((70, 24, 10), (0, 0, -5), "slate"),
         ((6, 6, 8), (0, 0, -14), "copper")]
make_piece("SM_Kit_CeilingLight", parts, bevel=0.6, glow_parts=[((60, 18, 4), (0, 0, -12))])

# generator: cabinet with a hazard base, ink vent block, copper lever on the -x face
parts = [((60, 90, 140), (0, 0, 70), "slate"),
         ((64, 94, 12), (0, 0, 6), "hazard"),
         ((6, 60, 40), (-31, 0, 110), "ink"),
         ((10, 8, 44), (-33, 30, 60), "copper"),
         ((12, 12, 12), (-36, 30, 84), "copper"),
         ((30, 30, 16), (0, 0, 148), "ink")]
make_piece("SM_Kit_Generator", parts, bevel=1.2)

# drone: a flat slate body on four ink rotor discs (boxes, low-poly), origin at the base
parts = [((60, 60, 14), (0, 0, 20), "slate"),
         ((16, 16, 8), (0, 0, 31), "copper")]
for dx in (-1, 1):
    for dy in (-1, 1):
        parts.append(((24, 24, 4), (dx * 36, dy * 36, 26), "ink"))
        parts.append(((6, 6, 12), (dx * 36, dy * 36, 18), "slate"))
make_piece("SM_Kit_Drone", parts, bevel=0.8)

# crate: slate box with bone signage strap and copper corners
parts = [((60, 60, 60), (0, 0, 30), "slate"),
         ((62, 62, 12), (0, 0, 30), "signage")]
make_piece("SM_Kit_Crate", parts, bevel=1.5)

# vent: a wall grille (face at x=-7, 80 x 80) with a hollow behind it; the fan is its own piece
parts = [((14, 80, 6), (-7, 0, 3), "slate"),
         ((14, 80, 6), (-7, 0, 77), "slate"),
         ((14, 6, 68), (-7, -37, 40), "slate"),
         ((14, 6, 68), (-7, 37, 40), "slate"),
         ((2, 68, 68), (-13, 0, 40), "ink"),
         ((4, 84, 4), (-14, 0, 2), "copper"),
         ((4, 84, 4), (-14, 0, 78), "copper")]
make_piece("SM_Kit_Vent", parts, bevel=0.8)

# vent fan: four blades and a hub, spinning about x; origin at the hub
parts = [((6, 12, 12), (0, 0, 0), "copper"),
         ((3, 60, 10), (0, 0, 0), "slate"),
         ((3, 10, 60), (0, 0, 0), "slate")]
make_piece("SM_Kit_VentFan", parts, bevel=0.5)

# --- Sectors 2 to 4 (Phase 9) ------------------------------------------------------------

# heater: a wall radiator, face at -x; slate housing, copper fins, ink grille slots, hazard foot
parts = [((16, 120, 80), (-8, 0, 40), "slate"),
         ((20, 124, 6), (-10, 0, 3), "hazard")]
for i in range(7):
    parts.append(((6, 8, 64), (-19, -48 + i * 16, 42), "copper"))
make_piece("SM_Kit_Heater", parts, bevel=0.6)

# sprinkler head: hangs from the ceiling (origin at the mount), copper stem and a bone deflector disc
parts = [((6, 6, 16), (0, 0, -8), "copper"),
         ((22, 22, 3), (0, 0, -17), "bone"),
         ((10, 10, 4), (0, 0, -20), "slate")]
make_piece("SM_Kit_Sprinkler", parts, bevel=0.4)

# sprinkler spray: a translucent-looking bone column that the actor scales from the head downward
# (origin at the top, the column hangs 200 below it); glow material so it reads as water in the dark
make_piece("SM_Kit_SprinklerSpray", [((2, 2, 2), (0, 0, -1), "bone")], bevel=0,
           glow_parts=[((28, 28, 200), (0, 0, -100))])

# grow light: a ceiling bar (origin at the mount), slate housing, a long glow strip
make_piece("SM_Kit_GrowLight", [((160, 20, 10), (0, 0, -5), "slate"),
                                ((6, 6, 8), (-60, 0, -14), "copper"),
                                ((6, 6, 8), (60, 0, -14), "copper")], bevel=0.5,
           glow_parts=[((150, 14, 4), (0, 0, -12))])

# planter: a trough on the floor, slate walls, copper rim, a grating bed
parts = [((200, 70, 40), (0, 0, 20), "slate"),
         ((204, 74, 4), (0, 0, 42), "copper"),
         ((190, 60, 4), (0, 0, 40), "grating"),
         ((204, 74, 6), (0, 0, 3), "hazard")]
make_piece("SM_Kit_Planter", parts, bevel=0.8)

# conveyor: a belt 400 long along x, origin at its centre on the floor; ink belt, slate frame, copper rollers
parts = [((400, 80, 8), (0, 0, 36), "ink"),
         ((400, 8, 40), (0, -44, 20), "slate"),
         ((400, 8, 40), (0, 44, 20), "slate"),
         ((404, 96, 6), (0, 0, 3), "hazard")]
for i in range(-4, 5):
    parts.append(((8, 90, 8), (i * 45, 0, 30), "copper"))
make_piece("SM_Kit_Conveyor", parts, bevel=0.6)

# shelf rack: three shelves against a wall (back at +x), slate uprights, bone shelves
parts = [((40, 8, 220), (0, -76, 110), "slate"),
         ((40, 8, 220), (0, 76, 110), "slate"),
         ((40, 160, 4), (0, 0, 2), "hazard")]
for z in (60, 130, 200):
    parts.append(((40, 152, 4), (0, 0, z), "bone"))
make_piece("SM_Kit_Shelf", parts, bevel=0.5)

# reactor core: a tall column with copper bands and a glow ring; origin at its base
parts = [((120, 120, 260), (0, 0, 130), "slate"),
         ((130, 130, 14), (0, 0, 7), "hazard"),
         ((126, 126, 8), (0, 0, 90), "copper"),
         ((126, 126, 8), (0, 0, 170), "copper"),
         ((60, 60, 20), (0, 0, 270), "ink")]
make_piece("SM_Kit_ReactorCore", parts, bevel=1.0, glow_parts=[((128, 128, 10), (0, 0, 130))])

# coolant pipe: 300 long along x, copper, with two slate brackets; origin at its centre
parts = [((300, 16, 16), (0, 0, 0), "copper"),
         ((12, 24, 24), (-100, 0, 0), "slate"),
         ((12, 24, 24), (100, 0, 0), "slate")]
make_piece("SM_Kit_CoolantPipe", parts, bevel=0.4)

# mast: the beacon that takes the report; a tall slate pole with copper crossbars and a glow tip
parts = [((14, 14, 280), (0, 0, 140), "slate"),
         ((40, 40, 8), (0, 0, 4), "hazard"),
         ((4, 90, 4), (0, 0, 200), "copper"),
         ((4, 60, 4), (0, 0, 240), "copper")]
make_piece("SM_Kit_Mast", parts, bevel=0.4, glow_parts=[((10, 10, 10), (0, 0, 285))])

# sledge rack: the rack by the west door, empty; slate brackets and a copper rail
parts = [((30, 8, 60), (0, -50, 30), "slate"),
         ((30, 8, 60), (0, 50, 30), "slate"),
         ((6, 116, 6), (-12, 0, 56), "copper")]
make_piece("SM_Kit_SledgeRack", parts, bevel=0.4)

# test cube from Phase 0 stays as the pipeline's canary
if not bpy.data.objects.get("SM_Kit_TestCube"):
    make_piece("SM_Kit_TestCube", [((100, 100, 100), (0, 0, 50), "slate")], bevel=1.0)

for m in list(bpy.data.materials):
    if m.users == 0 and m.name not in ("M_Kit_Trim", "M_Kit_Glow"):
        bpy.data.materials.remove(m)
for me in list(bpy.data.meshes):
    if me.users == 0:
        bpy.data.meshes.remove(me)
for im in list(bpy.data.images):
    if im.users == 0:
        bpy.data.images.remove(im)
bpy.ops.wm.save_mainfile()
print("KIT:", sorted(o.name for o in kit.all_objects), "| collision:", len(coll.all_objects))
