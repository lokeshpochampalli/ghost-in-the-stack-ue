# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/setup_phase2_content.py).
# Phase 2 content: binds the Interact action on the player Blueprint, creates the airlock
# door script asset, and builds the Sector 1 corridor level with one door, one terminal and
# one wall display. Safe to re-run: existing assets are updated, the level is rebuilt.
import unreal

EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# --- 1. the player's Interact action -------------------------------------------------------
ia = EAL.load_asset("/Game/Input/Actions/IA_Interact")
bp_path = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
bp_class = unreal.load_class(None, bp_path + ".BP_FirstPersonCharacter_C")
cdo = unreal.get_default_object(bp_class)
cdo.set_editor_property("interact_action", ia)
bp = EAL.load_asset(bp_path)
bp.modify()
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
print("character InteractAction:", cdo.get_editor_property("interact_action").get_name() if cdo.get_editor_property("interact_action") else None)

# --- 2. the door script --------------------------------------------------------------------
SCRIPT_SOURCE = """# airlock, inner door
# ilse: the pressure has to match the hab side before this
# will open. it does now. if it ever doesn't, DON'T force it.

pressure = read_sensor("airlock")
target = 4

if pressure == target:
    log("pressure matched, opening")
    open_door("inner")
else:
    log("pressure off, holding")
"""
if not EAL.does_directory_exist("/Game/Scripts"):
    EAL.make_directory("/Game/Scripts")
script_path = "/Game/Scripts/DA_Airlock_InnerDoor"
if EAL.does_asset_exist(script_path):
    script = EAL.load_asset(script_path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GitsScript)
    script = TOOLS.create_asset("DA_Airlock_InnerDoor", "/Game/Scripts", unreal.GitsScript, factory)
script.set_editor_property("title", "AIRLOCK 1  inner door")
script.set_editor_property("source", SCRIPT_SOURCE)
script.set_editor_property("tier", 2)
script.set_editor_property("statement_cap", 2000)
script.set_editor_property("editable_lines", [])
script.set_editor_property("declared_builtins", ["read_sensor", "log", "open_door", "close_door", "wait"])
EAL.save_asset(script_path)
print("script asset:", script.get_path_name(), "| lines:", len(SCRIPT_SOURCE.splitlines()))

# --- 3. the level ----------------------------------------------------------------------------
LEVEL = "/Game/Sectors/L_Sector1_Airlock"
if not EAL.does_directory_exist("/Game/Sectors"):
    EAL.make_directory("/Game/Sectors")
if EAL.does_asset_exist(LEVEL):
    LES.load_level(LEVEL)
    for a in EAS.get_all_level_actors():
        EAS.destroy_actor(a)
else:
    LES.new_level(LEVEL)

def mesh(name):
    return EAL.load_asset(f"/Game/Kit/{name}")

def rotator(rot):
    # rot is (pitch, yaw, roll) like the editor shows it; unreal.Rotator's positional order is (roll, pitch, yaw)
    return unreal.Rotator(roll=float(rot[2]), pitch=float(rot[0]), yaw=float(rot[1]))

def place_mesh(name, loc, rot=(0, 0, 0), label=None, scale=(1, 1, 1)):
    a = EAS.spawn_actor_from_object(mesh(name), unreal.Vector(*loc), rotator(rot))
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label or name)
    a.set_mobility(unreal.ComponentMobility.STATIC)
    return a

def spawn(cls, loc, rot=(0, 0, 0), label=None):
    a = EAS.spawn_actor_from_class(cls, unreal.Vector(*loc), rotator(rot))
    if label: a.set_actor_label(label)
    return a

# corridor: three segments then the airlock, then one more segment beyond the door
for i in range(3):
    place_mesh("SM_Kit_CorridorSegment", (i * 400, 0, 0), label=f"Corridor_{i+1}")
place_mesh("SM_Kit_CorridorSegment", (1230, 0, 0), label="Corridor_Beyond")
# a wall closing the far end so the beyond stays dark, not void
place_mesh("SM_Kit_DoorFrame", (1645, 0, 0), label="EndWall_Frame")
place_mesh("SM_Kit_DoorPanel", (1645, 0, 0), label="EndWall_Panel")
# and one closing the start behind the player
place_mesh("SM_Kit_DoorFrame", (-15, 0, 0), label="StartWall_Frame")
place_mesh("SM_Kit_DoorPanel", (-15, 0, 0), label="StartWall_Panel")

door = spawn(unreal.GitsDoor, (1215, 0, 0), label="Door_Inner")
door.frame.set_static_mesh(mesh("SM_Kit_DoorFrame"))
door.panel.set_static_mesh(mesh("SM_Kit_DoorPanel"))
door.set_editor_property("system_id", "inner")
door.set_editor_property("open_height", 215.0)
door.set_editor_property("speed", 140.0)

terminal = spawn(unreal.GitsTerminal, (1120, 105, 0), label="Terminal_Airlock")
terminal.body.set_static_mesh(mesh("SM_Kit_Terminal"))
terminal.set_editor_property("script", script)

display = spawn(unreal.GitsWallDisplay, (980, 149, 150), rot=(0, 90, 0), label="WallDisplay_Airlock")
display.body.set_static_mesh(mesh("SM_Kit_WallDisplay"))

station = spawn(unreal.GitsStation, (0, 0, 0), label="Station")
spec = unreal.GitsSensorSpec()
spec.set_editor_property("id", "airlock"); spec.set_editor_property("value", 4.0); spec.set_editor_property("drift_per_tick", 0.0)
station.set_editor_property("sensors", [spec])
station.set_editor_property("initial_switches", {"door.inner": False})

# lights: warm and low, the terminal does the rest
for i, x in enumerate((250, 650, 1050)):
    light = spawn(unreal.GitsLight, (x, 0, 292), label=f"Light_Corridor_{i+1}")
    light.set_editor_property("system_id", "corridor")
    light.set_editor_property("initial_level", 2.5)
    light.set_editor_property("full_intensity", 24.0)
beyond = spawn(unreal.GitsLight, (1430, 0, 292), label="Light_Beyond")
beyond.set_editor_property("system_id", "beyond")
beyond.set_editor_property("initial_level", 1.0)
beyond.set_editor_property("full_intensity", 24.0)

# a cold fill near the door so the copper panel reads
fill = spawn(unreal.PointLight, (1150, -120, 40), label="Fill_Cold")
fill.point_light_component.set_intensity(0.8)
fill.point_light_component.set_light_color(unreal.LinearColor(0.55, 0.7, 0.9, 1.0))
fill.point_light_component.set_attenuation_radius(500.0)
fill.point_light_component.set_mobility(unreal.ComponentMobility.STATIONARY)

# exposure pinned at EV100 0 so the screens (unlit widgets, emissive 1.0) render at full
# brightness and the corridor is lit by deliberately weak lamps (candela values above).
ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess")
ppv.set_editor_property("unbound", True)
settings = ppv.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 0.0)
settings.set_editor_property("auto_exposure_max_brightness", 0.0)
ppv.set_editor_property("settings", settings)

start = spawn(unreal.PlayerStart, (150, 0, 100), rot=(0, 0, 0), label="PlayerStart")

LES.save_current_level()
print("level saved:", LEVEL, "| actors:", len(EAS.get_all_level_actors()))
