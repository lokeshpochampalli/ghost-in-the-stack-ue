# Shared helpers for the sector content scripts (Sectors 2 to 4). Runs inside Unreal's Python.
# A content script does:
#     import sys; sys.path.append(r"D:\...\Tools"); from gits_content_lib import *
# then authors its scripts with ensure_script(...) and builds its level with the placers.
import unreal

EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def rotator(rot):
    # rot is (pitch, yaw, roll) like the editor shows it; unreal.Rotator's positional order is (roll, pitch, yaw)
    return unreal.Rotator(roll=float(rot[2]), pitch=float(rot[0]), yaw=float(rot[1]))


# --- scripts -------------------------------------------------------------------------------

def option(oid, label, misconception="", reveal=""):
    o = unreal.GitsPredictionOption()
    o.set_editor_property("id", oid)
    o.set_editor_property("label", label)
    o.set_editor_property("misconception", misconception)
    o.set_editor_property("reveal", reveal)
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


def test_case(label, output=(), world=None):
    t = unreal.GitsTestCase()
    t.set_editor_property("label", label)
    t.set_editor_property("expected_output", list(output))
    t.set_editor_property("expected_world", dict(world or {}))
    return t


def ensure_script(name, title, source, builtins, c):
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
    script.set_editor_property("tier", c.get("tier", 1))
    script.set_editor_property("statement_cap", 2000)
    script.set_editor_property("editable_lines", c.get("editable_lines", []))
    script.set_editor_property("declared_builtins", builtins)
    script.set_editor_property("concepts", c["concepts"])
    script.set_editor_property("predictions", c.get("predictions", []))
    script.set_editor_property("hints", c["hints"])
    script.set_editor_property("intro", c["intro"])
    script.set_editor_property("outro", c["outro"])
    script.set_editor_property("goal_key", c.get("goal_key", ""))
    script.set_editor_property("goal_value", c.get("goal_value", ""))
    script.set_editor_property("goal_output", c.get("goal_output", []))
    script.set_editor_property("test_cases", c.get("tests", []))
    script.set_editor_property("goal_unlocks", c.get("unlocks", ""))
    script.set_editor_property("counts_for_sector", c.get("counts", True))
    script.set_editor_property("log_entry", c.get("log", ""))
    script.set_editor_property("free_edit", c.get("free_edit", False))
    script.set_editor_property("panel_title", c.get("panel", ""))
    script.set_editor_property("level_id", c.get("level_id", ""))
    script.set_editor_property("act", c.get("act", 1))
    script.set_editor_property("run_cost", c.get("run_cost", 10))
    script.set_editor_property("predicted_run_cost", c.get("predicted_run_cost", 3))
    EAL.save_asset(path)
    print("script asset:", script.get_path_name(), "| lines:", len(source.splitlines()))
    return script


# --- the level -----------------------------------------------------------------------------

def begin_level(path):
    if not EAL.does_directory_exist("/Game/Sectors"):
        EAL.make_directory("/Game/Sectors")
    if EAL.does_asset_exist(path):
        LES.load_level(path)
        for a in EAS.get_all_level_actors():
            EAS.destroy_actor(a)
    else:
        LES.new_level(path)


def save_level(path):
    LES.save_current_level()
    print("level saved:", path, "| actors:", len(EAS.get_all_level_actors()))


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


def terminal(loc, yaw, script, label):
    t = spawn(unreal.GitsTerminal, loc, rot=(0, yaw, 0), label=label)
    t.body.set_static_mesh(mesh("SM_Kit_Terminal"))
    t.set_editor_property("script", script)
    return t


def panel(loc, yaw, script, label, scale=1.0):
    d = spawn(unreal.GitsWallDisplay, loc, rot=(0, yaw, 0), label=label)
    d.body.set_static_mesh(mesh("SM_Kit_WallDisplay"))
    d.set_editor_property("script", script)
    if scale != 1.0:
        d.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    return d


def gits_door(loc, yaw, system_id, label, with_frame=True):
    d = spawn(unreal.GitsDoor, loc, rot=(0, yaw, 0), label=label)
    if with_frame:
        d.frame.set_static_mesh(mesh("SM_Kit_DoorFrame"))
    d.panel.set_static_mesh(mesh("SM_Kit_DoorPanel"))
    d.set_editor_property("system_id", system_id)
    d.set_editor_property("open_height", 215.0)
    d.set_editor_property("speed", 140.0)
    return d


def sector_gate(loc, yaw, door_key, next_level, label_text, label, sealed_line=None):
    g = spawn(unreal.GitsSectorGate, loc, rot=(0, yaw, 0), label=label)
    g.frame.set_static_mesh(mesh("SM_Kit_DoorFrame"))
    g.panel.set_static_mesh(mesh("SM_Kit_DoorPanel"))
    g.set_editor_property("door_key", door_key)
    g.set_editor_property("next_level", next_level)
    g.set_editor_property("label", label_text)
    if sealed_line:
        g.set_editor_property("sealed_line", sealed_line)
    return g


def ceiling_light(loc, system_id, level, label, full=12.0, color=None, fitting="SM_Kit_CeilingLight", beacon=False):
    light = spawn(unreal.GitsLight, loc, label=label)
    if fitting:
        light.fitting.set_static_mesh(mesh(fitting))
    light.set_editor_property("system_id", system_id)
    light.set_editor_property("initial_level", level)
    light.set_editor_property("full_intensity", full)
    light.set_editor_property("beacon", beacon)
    if color:
        light.light.set_light_color(unreal.LinearColor(*color))
    return light


def lamp(loc, system_id, label, full=6.0, color=(1.0, 0.55, 0.2, 1.0), level=0.0, beacon=False):
    """A bare indicator light with no fitting: an alarm lamp, a beacon, a glow."""
    return ceiling_light(loc, system_id, level, label, full=full, color=color, fitting=None, beacon=beacon)


def emergency_lights(locs):
    for i, loc in enumerate(locs):
        e = spawn(unreal.GitsLight, loc, label=f"Light_Emergency_{i+1}")
        e.set_editor_property("system_id", "emergency")
        e.set_editor_property("initial_level", 10.0)
        e.set_editor_property("full_intensity", 1.5)
        e.set_editor_property("emergency_only", True)
        e.light.set_light_color(unreal.LinearColor(0.85, 0.60, 0.17, 1.0))


def fill_light(loc, intensity, radius, label, color=(0.55, 0.7, 0.9, 1.0)):
    fill = spawn(unreal.PointLight, loc, label=label)
    fill.point_light_component.set_intensity(intensity)
    fill.point_light_component.set_light_color(unreal.LinearColor(*color))
    fill.point_light_component.set_attenuation_radius(radius)
    fill.point_light_component.set_mobility(unreal.ComponentMobility.STATIONARY)
    return fill


def heater(loc, yaw, system_id, label):
    h = spawn(unreal.GitsHeater, loc, rot=(0, yaw, 0), label=label)
    h.body.set_static_mesh(mesh("SM_Kit_Heater"))
    h.set_editor_property("system_id", system_id)
    return h


def sprinkler(loc, system_id, label):
    s = spawn(unreal.GitsSprinkler, loc, label=label)
    s.head.set_static_mesh(mesh("SM_Kit_Sprinkler"))
    s.spray.set_static_mesh(mesh("SM_Kit_SprinklerSpray"))
    s.set_editor_property("system_id", system_id)
    return s


def conveyor(loc, yaw, system_id, label, crates=3):
    c = spawn(unreal.GitsConveyor, loc, rot=(0, yaw, 0), label=label)
    c.belt.set_static_mesh(mesh("SM_Kit_Conveyor"))
    c.set_editor_property("crate_mesh", mesh("SM_Kit_Crate"))
    c.set_editor_property("crate_count", crates)
    c.set_editor_property("system_id", system_id)
    return c


def vent(loc, yaw, system_id, label):
    v = spawn(unreal.GitsVent, loc, rot=(0, yaw, 0), label=label)
    v.grille.set_static_mesh(mesh("SM_Kit_Vent"))
    v.fan.set_static_mesh(mesh("SM_Kit_VentFan"))
    v.set_editor_property("system_id", system_id)
    return v


def wall_gauge(loc, yaw, label):
    g = spawn(unreal.GitsPowerGauge, loc, rot=(0, yaw, 0), label=label)
    g.body.set_static_mesh(mesh("SM_Kit_WallGauge"))
    g.set_actor_scale3d(unreal.Vector(0.5, 0.5, 0.5))
    return g


def generator(loc, yaw, label="Generator"):
    g = spawn(unreal.GitsGenerator, loc, rot=(0, yaw, 0), label=label)
    g.body.set_static_mesh(mesh("SM_Kit_Generator"))
    return g


def station(switches, levels, budget, reserve, complete_line, unlocks, sensors=()):
    st = spawn(unreal.GitsStation, (0, 0, 0), label="Station")
    specs = []
    for sid, value, drift in sensors:
        spec = unreal.GitsSensorSpec()
        spec.set_editor_property("id", sid); spec.set_editor_property("value", float(value)); spec.set_editor_property("drift_per_tick", float(drift))
        specs.append(spec)
    st.set_editor_property("sensors", specs)
    st.set_editor_property("initial_switches", switches)
    st.set_editor_property("initial_levels", levels)
    st.set_editor_property("power_budget", budget)
    st.set_editor_property("reserve_restore", reserve)
    st.set_editor_property("sector_complete_line", complete_line)
    st.set_editor_property("sector_unlocks", unlocks)
    return st


def exposure_and_start(start_loc, start_yaw=0):
    ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_min_brightness", True)
    settings.set_editor_property("override_auto_exposure_max_brightness", True)
    settings.set_editor_property("auto_exposure_min_brightness", 0.0)
    settings.set_editor_property("auto_exposure_max_brightness", 0.0)
    ppv.set_editor_property("settings", settings)
    spawn(unreal.PlayerStart, start_loc, rot=(0, start_yaw, 0), label="PlayerStart")


# --- corridor building --------------------------------------------------------------------
# Kit pieces: a corridor segment is 400 long along +x from its origin, 300 wide, 300 high.
# A doorway segment has its opening on +y (yaw 180 puts it on -y). A corner occupies x 0..300
# and turns from +x to +y. A wall cap closes an open end.

def run_x(x0, y, count, label, z=0):
    """Straight run of `count` segments from x0 along +x at the given y."""
    for i in range(count):
        place_mesh("SM_Kit_CorridorSegment", (x0 + i * 400, y, z), label=f"{label}_{i+1}")


def room_off(x, y, side, label):
    """A side room: one segment running away from the corridor on the given side (+1 north, -1 south), capped."""
    yaw = 90 if side > 0 else -90
    place_mesh("SM_Kit_CorridorSegment", (x, y + side * 150, 0), rot=(0, yaw, 0), label=label)
    place_mesh("SM_Kit_WallCap", (x, y + side * 550, 0), rot=(0, yaw, 0), label=label + "_Cap")
