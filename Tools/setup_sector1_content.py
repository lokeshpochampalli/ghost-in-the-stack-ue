# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/setup_sector1_content.py).
# Sector 1 content, Phases 2 to 4: binds the Interact and Rewind actions on the player
# Blueprint, creates the script assets with VANT's predictions, Ilse's hints and the goals,
# and builds the Sector 1 corridor level with two terminals (the airlock door, the corridor
# lights), one door, one wall display.
# Safe to re-run: existing assets are updated, the level is rebuilt.
import unreal

EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def rotator(rot):
    # rot is (pitch, yaw, roll) like the editor shows it; unreal.Rotator's positional order is (roll, pitch, yaw)
    return unreal.Rotator(roll=float(rot[2]), pitch=float(rot[0]), yaw=float(rot[1]))


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


# --- 1. input actions: Interact (E, gamepad face-left) and Rewind (hold R, gamepad left shoulder)
def ensure_action(name):
    path = "/Game/Input/Actions/" + name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    factory = unreal.InputActionFactory() if hasattr(unreal, "InputActionFactory") else None
    asset = TOOLS.create_asset(name, "/Game/Input/Actions", unreal.InputAction, factory)
    asset.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    EAL.save_asset(path)
    return asset


def ensure_mapping(imc, action, key_names):
    have = set()
    # the live data is in default_key_mappings; the top-level "mappings" property is the deprecated view
    for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings"):
        a = m.get_editor_property("action")
        if a and a.get_name() == action.get_name():
            have.add(str(m.get_editor_property("key").get_editor_property("key_name")))
    for k in key_names:
        if k not in have:
            imc.map_key(action, key(k))
    imc.modify()
    EAL.save_loaded_asset(imc, only_if_is_dirty=False)


ia_interact = ensure_action("IA_Interact")
ia_rewind = ensure_action("IA_Rewind")
imc = EAL.load_asset("/Game/Input/IMC_Default")
ensure_mapping(imc, ia_interact, ["E", "Gamepad_FaceButton_Left"])
ensure_mapping(imc, ia_rewind, ["R", "Gamepad_LeftShoulder"])

bp_path = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
bp_class = unreal.load_class(None, bp_path + ".BP_FirstPersonCharacter_C")
cdo = unreal.get_default_object(bp_class)
cdo.set_editor_property("interact_action", ia_interact)
cdo.set_editor_property("rewind_action", ia_rewind)
bp = EAL.load_asset(bp_path)
bp.modify()
EAL.save_loaded_asset(bp, only_if_is_dirty=False)
print("character actions:", cdo.get_editor_property("interact_action").get_name(), cdo.get_editor_property("rewind_action").get_name())

# --- 2. the scripts --------------------------------------------------------------------------
# Ilse's door script ships with her bug: the hab side was re-trimmed to 4 after she wrote 5,
# so the comparison on line 8 holds the door. The player reads it, predicts, watches, fixes line 6.
DOOR_SOURCE = """# airlock, inner door
# ilse: the pressure has to match the hab side before this
# will open. it does now. if it ever doesn't, DON'T force it.

pressure = read_sensor("airlock")
target = 5

if pressure == target:
    log("pressure matched, opening")
    open_door("inner")
else:
    log("pressure off, holding")
"""

LIGHTS_SOURCE = """# corridor lights
# ilse: the old ballasts pop if you slam them on. bring them
# up a notch at a time and let each notch settle.

level = 0
for notch in range(9):
    level = level + 1
    set_light("corridor", level)
    for tick in range(3):
        wait(1)
log("corridor lit")
"""


def option(oid, label, misconception=""):
    o = unreal.GitsPredictionOption()
    o.set_editor_property("id", oid)
    o.set_editor_property("label", label)
    o.set_editor_property("misconception", misconception)
    return o


def prediction(pid, prompt, line, occurrence, kind, options, correct):
    p = unreal.GitsPrediction()
    p.set_editor_property("id", pid)
    p.set_editor_property("prompt", prompt)
    p.set_editor_property("anchor_line", line)
    p.set_editor_property("anchor_occurrence", occurrence)
    p.set_editor_property("kind", kind)
    p.set_editor_property("options", options)
    p.set_editor_property("correct_id", correct)
    return p


def hint(tier, text, costs=0):
    h = unreal.GitsHint()
    h.set_editor_property("tier", tier)
    h.set_editor_property("text", text)
    h.set_editor_property("costs_power", costs)
    return h


DOOR_CURRICULUM = dict(
    concepts=["variable-assignment", "comparison", "conditional"],
    predictions=[prediction(
        "p1",
        "Before you run that. Line 8 compares them. Which way does it go?",
        8, 1, "branch",
        [
            option("a", "To line 12. The sensor says 4, target says 5, so it holds."),
            option("b", "To line 9. read_sensor sets target to match the pressure.", "parallel-assignment"),
            option("c", "To line 9. == makes pressure equal to target, so they match.", "assignment-as-equality"),
            option("d", "Both. It opens on line 10, then logs the hold on line 12.", "branch-both"),
        ],
        "a",
    )],
    hints=[
        hint(1, "the hab side got re-trimmed in august. check what the sensor actually reads."),
        hint(2, "line 6 is a number I typed. line 5 is a number the station measures. they have to agree, and only one of them is mine to change.", 3),
        hint(3, "target = 4.", 6),
    ],
    intro="Airlock one. Ilse's script, her comment, her bug. Read it before you spend the power; I have been alone with this door for eleven months.",
    outro="Inner door released. That is the first thing to work in this corridor since she left.",
    goal_key="door.inner",
    goal_value="true",
)

LIGHTS_CURRICULUM = dict(
    concepts=["for-loop", "range", "nested-loop", "accumulator"],
    predictions=[prediction(
        "p1",
        "Line 10, the wait. How many times does it run before the log on line 11?",
        11, 1, "count",
        [
            option("a", "27. Three waits for each of nine notches."),
            option("b", "9. Once per notch; the inner loop runs once.", "loop-runs-once"),
            option("c", "30. range(9) counts to nine inclusive, three each.", "fencepost"),
        ],
        "a",
    )],
    hints=[
        hint(1, "two loops. the inner one runs to the end every time the outer one comes round."),
        hint(2, "range(9) gives 0 to 8, nine notches. range(3) gives three ticks inside each. count them up.", 3),
    ],
    intro="Corridor lights. She wrote this one gently; the ballasts are older than both of us.",
    outro="Corridor lit, all nine notches. The ballasts held.",
    goal_key="light.corridor",
    goal_value="9",
)


def ensure_script(name, title, source, builtins, curriculum):
    if not EAL.does_directory_exist("/Game/Scripts"):
        EAL.make_directory("/Game/Scripts")
    path = "/Game/Scripts/" + name
    if EAL.does_asset_exist(path):
        script = EAL.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.GitsScript)
        script = TOOLS.create_asset(name, "/Game/Scripts", unreal.GitsScript, factory)
    script.set_editor_property("title", title)
    script.set_editor_property("source", source)
    script.set_editor_property("tier", 3)
    script.set_editor_property("statement_cap", 2000)
    script.set_editor_property("editable_lines", [])
    script.set_editor_property("declared_builtins", builtins)
    script.set_editor_property("concepts", curriculum["concepts"])
    script.set_editor_property("predictions", curriculum["predictions"])
    script.set_editor_property("hints", curriculum["hints"])
    script.set_editor_property("intro", curriculum["intro"])
    script.set_editor_property("outro", curriculum["outro"])
    script.set_editor_property("goal_key", curriculum["goal_key"])
    script.set_editor_property("goal_value", curriculum["goal_value"])
    # the power economy (ADR-006): full price, and the price once a reading is committed
    script.set_editor_property("run_cost", curriculum.get("run_cost", 12))
    script.set_editor_property("predicted_run_cost", curriculum.get("predicted_run_cost", 4))
    EAL.save_asset(path)
    print("script asset:", script.get_path_name(), "| lines:", len(source.splitlines()))
    return script


door_script = ensure_script("DA_Airlock_InnerDoor", "AIRLOCK 1  inner door", DOOR_SOURCE,
                            ["read_sensor", "log", "open_door", "close_door", "wait"], DOOR_CURRICULUM)
lights_script = ensure_script("DA_Corridor_Lights", "CORRIDOR 1  lights", LIGHTS_SOURCE,
                              ["log", "set_light", "wait", "range"], LIGHTS_CURRICULUM)

# --- 3. the level ----------------------------------------------------------------------------
# Sector 1, Habitation (Phase 6 blockout): the entry airlock, a main corridor with two side rooms
# (the generator room, the cold store), a corner, the airlock to Sector 2 and the sealed door
# beyond it. Kit pieces are 400 x 300 x 300 corridor segments; rotations turn them into rooms.
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


def place_mesh(name, loc, rot=(0, 0, 0), label=None, scale=(1, 1, 1)):
    a = EAS.spawn_actor_from_object(mesh(name), unreal.Vector(*loc), rotator(rot))
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label or name)
    a.set_mobility(unreal.ComponentMobility.STATIC)
    return a


def spawn(cls, loc, rot=(0, 0, 0), label=None):
    a = EAS.spawn_actor_from_class(cls, unreal.Vector(*loc), rotator(rot))
    if label:
        a.set_actor_label(label)
    return a


def sealed_door(loc, yaw, label):
    place_mesh("SM_Kit_DoorFrame", loc, rot=(0, yaw, 0), label=label + "_Frame")
    place_mesh("SM_Kit_DoorPanel", loc, rot=(0, yaw, 0), label=label + "_Panel")


# the main run: entry hall, two doorway segments (one opening each side), the corner, the leg to the airlock
place_mesh("SM_Kit_CorridorSegment", (0, 0, 0), label="Corridor_A")
place_mesh("SM_Kit_CorridorDoorway", (400, 0, 0), label="Corridor_B_DoorwayNorth")
place_mesh("SM_Kit_CorridorDoorway", (1200, 0, 0), rot=(0, 180, 0), label="Corridor_C_DoorwaySouth")
place_mesh("SM_Kit_CorridorCorner", (1200, 0, 0), label="Corner")
place_mesh("SM_Kit_CorridorSegment", (1350, 150, 0), rot=(0, 90, 0), label="Corridor_D_ToAirlock")
place_mesh("SM_Kit_CorridorSegment", (1350, 580, 0), rot=(0, 90, 0), label="Corridor_E_Beyond")
sealed_door((-15, 0, 0), 0, "EntryAirlock_Outer")
sealed_door((1350, 995, 0), 90, "Sector2_Sealed")

# side rooms: a segment turned across the corridor, capped at the far end
place_mesh("SM_Kit_CorridorSegment", (600, 150, 0), rot=(0, 90, 0), label="Room_Generator")
place_mesh("SM_Kit_WallCap", (600, 550, 0), rot=(0, 90, 0), label="Room_Generator_Cap")
place_mesh("SM_Kit_CorridorSegment", (1000, -150, 0), rot=(0, -90, 0), label="Room_ColdStore")
place_mesh("SM_Kit_WallCap", (1000, -550, 0), rot=(0, -90, 0), label="Room_ColdStore_Cap")

# the airlock door into Sector 2's approach, driven by the door script
door = spawn(unreal.GitsDoor, (1350, 565, 0), rot=(0, 90, 0), label="Door_Inner")
door.frame.set_static_mesh(mesh("SM_Kit_DoorFrame"))
door.panel.set_static_mesh(mesh("SM_Kit_DoorPanel"))
door.set_editor_property("system_id", "inner")
door.set_editor_property("open_height", 215.0)
door.set_editor_property("speed", 140.0)

terminal = spawn(unreal.GitsTerminal, (1480, 400, 0), label="Terminal_Airlock")
terminal.body.set_static_mesh(mesh("SM_Kit_Terminal"))
terminal.set_editor_property("script", door_script)

lights_terminal = spawn(unreal.GitsTerminal, (320, 128, 0), rot=(0, 90, 0), label="Terminal_Lights")
lights_terminal.body.set_static_mesh(mesh("SM_Kit_Terminal"))
lights_terminal.set_editor_property("script", lights_script)

display = spawn(unreal.GitsWallDisplay, (1494, 300, 150), label="WallDisplay_Airlock")
display.body.set_static_mesh(mesh("SM_Kit_WallDisplay"))

gauge = spawn(unreal.GitsPowerGauge, (150, 149, 170), rot=(0, 90, 0), label="PowerGauge")
gauge.body.set_static_mesh(mesh("SM_Kit_WallGauge"))
gauge.set_actor_scale3d(unreal.Vector(0.5, 0.5, 0.5))

generator = spawn(unreal.GitsGenerator, (600, 520, 0), rot=(0, 90, 0), label="Generator")
generator.body.set_static_mesh(mesh("SM_Kit_Generator"))

# dressing: the cold store unit, crates, and the drone that will fly in Sector 3, parked
place_mesh("SM_Kit_ColdStoreUnit", (1000, -515, 0), rot=(0, -90, 0), label="ColdStoreUnit")
place_mesh("SM_Kit_Crate", (890, -430, 0), rot=(0, 8, 0), label="Crate_1")
place_mesh("SM_Kit_Crate", (890, -365, 0), rot=(0, -5, 0), label="Crate_2")
place_mesh("SM_Kit_Crate", (890, -430, 60), rot=(0, 20, 0), label="Crate_3")
place_mesh("SM_Kit_Crate", (1110, -300, 0), rot=(0, 3, 0), label="Crate_4")
place_mesh("SM_Kit_Drone", (700, 330, 0), rot=(0, 25, 0), label="Drone_Parked")

station = spawn(unreal.GitsStation, (0, 0, 0), label="Station")
spec = unreal.GitsSensorSpec()
spec.set_editor_property("id", "airlock"); spec.set_editor_property("value", 4.0); spec.set_editor_property("drift_per_tick", 0.0)
station.set_editor_property("sensors", [spec])
station.set_editor_property("initial_switches", {"door.inner": False})
# the bus: one predicted run (4) and three full-price runs (12) drain it exactly; the reserve brings back two full runs
station.set_editor_property("power_budget", 40)
station.set_editor_property("reserve_restore", 24)


def ceiling_light(loc, system_id, level, label, full=12.0):
    light = spawn(unreal.GitsLight, loc, label=label)
    light.fitting.set_static_mesh(mesh("SM_Kit_CeilingLight"))
    light.set_editor_property("system_id", system_id)
    light.set_editor_property("initial_level", level)
    light.set_editor_property("full_intensity", full)
    return light


# the rig: fittings hang from the ceiling (origin at the mount), warm and low; the lights script brings the corridor up
for i, loc in enumerate(((200, 0, 299), (600, 0, 299), (1000, 0, 299), (1350, 0, 299), (1350, 350, 299))):
    ceiling_light(loc, "corridor", 2.5, f"Light_Corridor_{i+1}")
ceiling_light((600, 380, 299), "hab", 4.5, "Light_GeneratorRoom")
ceiling_light((1000, -380, 299), "cold", 2.5, "Light_ColdStore")
ceiling_light((1350, 780, 299), "beyond", 1.0, "Light_Beyond")

# emergency lighting: amber, and only when the bus is out
for i, loc in enumerate(((400, 0, 285), (1350, 200, 285), (1000, -300, 285))):
    e = spawn(unreal.GitsLight, loc, label=f"Light_Emergency_{i+1}")
    e.set_editor_property("system_id", "emergency")
    e.set_editor_property("initial_level", 10.0)
    e.set_editor_property("full_intensity", 1.5)
    e.set_editor_property("emergency_only", True)
    e.light.set_light_color(unreal.LinearColor(0.85, 0.60, 0.17, 1.0))


def cold_fill(loc, intensity, radius, label):
    fill = spawn(unreal.PointLight, loc, label=label)
    fill.point_light_component.set_intensity(intensity)
    fill.point_light_component.set_light_color(unreal.LinearColor(0.55, 0.7, 0.9, 1.0))
    fill.point_light_component.set_attenuation_radius(radius)
    fill.point_light_component.set_mobility(unreal.ComponentMobility.STATIONARY)
    return fill


# cold fills: the frost in the cold store, the dark beyond the airlock, the entry door
cold_fill((1000, -420, 200), 2.5, 500.0, "Fill_ColdStore")
cold_fill((1350, 900, 140), 1.2, 500.0, "Fill_Beyond")
cold_fill((60, -110, 40), 0.8, 400.0, "Fill_Entry")

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
