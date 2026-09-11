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
# Sector 1's eight systems, re-authored from the reference's Act 1 (a1-l01 to a1-l08). Each keeps
# its concept and misconception tags; every wrong reading has its physical failure on a panel
# and its named reveal in VANT's voice. Line numbers in prompts and anchors are the lines below.

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
    script.set_editor_property("run_cost", c.get("run_cost", 10))
    script.set_editor_property("predicted_run_cost", c.get("predicted_run_cost", 3))
    EAL.save_asset(path)
    print("script asset:", script.get_path_name(), "| lines:", len(source.splitlines()))
    return script


# 1. the door log (a1-l01): variable-assignment, output. The entry interlock releases when the log reports.
S1 = ensure_script("DA_S1_DoorLog", "AIRLOCK 1  door log", """# main door counter
# ilse, day 3
# a name on the left, a value on the right. the name keeps the value.

opened = 7

# print shows what the name is keeping.
print(opened)
""", ["print"], dict(
    concepts=["variable-assignment", "output"],
    predictions=[prediction("p1", "Before you spend the power: what will line 8 put on the log?", 8, 1, "output", [
        option("a", "7"),
        option("b", "opened", "type-confusion", "It printed the name, not what the name keeps. The interlock wants a number."),
        option("c", "0", "sequence-ignored", "Line 5 ran before line 8; the name was keeping 7 by then."),
    ], "a")],
    hints=[hint(1, "Ilse's own note is on line 3. She wrote it for herself and it still holds."),
           hint(2, "Line 5 puts 7 under the name opened. Line 8 asks for whatever opened is keeping, and shows that.", 3)],
    intro="Nothing in the airlock works until the door log reports. It is the first script Ilse wrote here and it is four lines long. Read it. You have power for one run and no reason to waste it.",
    outro="The log reports. The inner door releases.",
    log="Day 3. First script I have written for this place. It counts the door. That is all it does, and the man who trained me said that is a start. I have written down what every line means because I do not trust myself to remember.",
    goal_output=["7"], unlocks="door.entry=true", panel="DOOR LOG",
))

# 2. the two gauges (a1-l02): reassignment. The pressure display reads what west was told.
S2 = ensure_script("DA_S1_GaugeMirror", "PRESSURE  gauge mirror", """# gauge mirror
# ilse, day 5
# west is broken. i copy east across so the panel stops complaining.

east = 12
west = east

# east has moved since i wrote this.
east = 15

print(west)
""", ["print"], dict(
    concepts=["variable-assignment", "reassignment", "output"],
    predictions=[prediction("p1", "What will line 11 report for west?", 11, 1, "output", [
        option("a", "12"),
        option("b", "15", "parallel-assignment", "West took a copy of east on line 6. It did not take a wire to it; east moving later moves nothing."),
        option("c", "27", "operator-confusion", "Nothing on this panel adds. Line 6 copies."),
        option("d", "Nothing, west was never given a number of its own", "sequence-ignored", "Line 6 gave west a number: the one east was keeping at that moment."),
    ], "a")],
    hints=[hint(1, "Ilse, in the margin: 'it is a copy i took at a moment, not the same gauge.'"),
           hint(2, "Line 6 asks east what it is keeping right then, and hands that value to west. After that the two names have nothing to do with each other.", 3)],
    intro="The west gauge died before Ilse arrived. She never replaced it; she copied the east reading across by hand and let the script write it down. The east reading has moved since. The script has not.",
    outro="West reads what it was told to read. Whether that is the truth is a separate question, and not one the station asks.",
    log="Day 5. Copied the east gauge into the west one so the panel would stop complaining. It is not the same gauge and I know that. It is a number I took at a moment.",
    goal_output=["12"], unlocks="gauge.west=12", panel="PRESSURE  west",
))

# 3. the cold store (a1-l03): reassignment with arithmetic. The compressor settles when the setpoint reads true.
S3 = ensure_script("DA_S1_ColdStore", "COLD STORE  setpoint", """# cold store setpoint report
# ilse, day 40

base_temp = 4
adjust = 2
base_temp = base_temp - adjust

print(base_temp)
""", ["print"], dict(
    concepts=["variable-assignment", "reassignment", "arithmetic", "output"],
    predictions=[prediction("p1", "What will line 8 print?", 8, 1, "output", [
        option("a", "2"),
        option("b", "4", "assignment-as-equality", "Line 6 is not a claim about base_temp. It is an instruction: work out the right side, store it under the name."),
        option("c", "6", "operator-confusion", "Adjust sounds like something you add. The line subtracts."),
        option("d", "Nothing, the line is invalid", "type-confusion", "The same name on both sides is the ordinary way to change a number. The panel got 2."),
    ], "a")],
    hints=[hint(1, "Ilse's note in the margin: 'the same name twice is not a contradiction, it is a sequence.'"),
           hint(2, "Line 6 reads the current value of base_temp, subtracts adjust, and stores the answer back under the same name.", 4)],
    intro="The cold store is four degrees warmer than it should be. Ilse left a script that reports the setpoint. Read it before you run it; the compressor draws hard on startup and you have not got the power to guess.",
    outro="Setpoint reads true. The compressor settles.",
    log="Day 40. Rewrote the cold store report because I could not remember what the old one did. Note to self: a comment costs nothing.",
    goal_output=["2"], unlocks="light.coldstore=8", panel="COLD STORE  setpoint", run_cost=12, predicted_run_cost=4,
))

# 4. the reclaimer panel (a1-l04): data types, output order. The vent runs once the panel takes all three.
S4 = ensure_script("DA_S1_Reclaimer", "RECLAIMER  panel readout", """# reclaimer panel readout
# ilse, day 44
# three kinds of value. the panel wants them in this order.

litres = 40
rate = 2.5
running = True

print(running)
print(litres)
print(rate)
""", ["print"], dict(
    concepts=["data-types", "output"],
    predictions=[
        prediction("p1", "What will line 9 put on the panel first?", 9, 1, "output", [
            option("a", "True"),
            option("b", "true", "type-confusion", "A yes-or-no value prints with a capital. The panel is fussy about that."),
            option("c", "40", "sequence-ignored", "Line 9 runs first and it prints running, not litres."),
            option("d", "running", "type-confusion", "It printed what the name keeps, not the name."),
        ], "a"),
        prediction("p2", "And what will line 11 put there last?", 11, 1, "output", [
            option("a", "2.5"),
            option("b", "2", "type-confusion", "A rate with a point in it keeps its point. Nothing rounds it."),
            option("c", "40", "sequence-ignored", "Line 11 prints rate. Litres went out on line 10."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'a number is not a word and neither of them is a yes.' Three kinds, three lines."),
           hint(2, "The panel shows each value the way that kind of value is written. A true/false value shows as True or False, a whole number as itself, a number with a point with its point.", 4)],
    intro="The water reclaimer's panel reads three lines and it reads them in the order it is given them. Ilse found that out the hard way and left the finding in a comment. One of the three is not a number at all.",
    outro="The panel takes all three and settles. The reclaimer starts its cycle.",
    log="Day 44. The panel faults if the lines come in the wrong order, and it faults differently if a line is the wrong kind of thing. Three kinds. I have written them down.",
    goal_output=["True", "40", "2.5"], unlocks="vent.reclaimer=true", panel="RECLAIMER",
))

# 5. the shift clock (a1-l05, the mixer, re-themed): the two divisions. The clock starts on the whole hours.
S5 = ensure_script("DA_S1_ShiftClock", "SHIFT CLOCK  hours", """# shift clock
# ilse, day 51
# one slash is the true share. two slashes is what the clock can show.

hours = 7
shifts = 2

print(hours / shifts)
print(hours // shifts)
""", ["print"], dict(
    concepts=["arithmetic", "type-coercion"],
    predictions=[
        prediction("p1", "Line 8 uses one slash. What comes out?", 8, 1, "output", [
            option("a", "3.5"),
            option("b", "3", "type-confusion", "One slash keeps the half. It always answers with a point in it."),
            option("c", "4", "operator-confusion", "Nothing rounds up here. Seven over two is three and a half."),
        ], "a"),
        prediction("p2", "Line 9 uses two. What comes out of that one?", 9, 1, "output", [
            option("a", "3"),
            option("b", "3.5", "operator-confusion", "Two slashes throw the remainder away. That is what they are for."),
            option("c", "3.0", "type-confusion", "Two slashes on two whole numbers give a whole number, no point."),
            option("d", "4", "fencepost", "Down, not up. Three whole hours, and the half is dropped."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'one slash gives you the truth and two slashes give you what the clock can show.'"),
           hint(2, "One slash always answers with a decimal, even when it divides evenly. Two slashes drop everything after the point and answer with a whole number.", 4)],
    intro="Seven hours, two shifts. The clock wants the split twice: once as it really is, once in whole hours, because the dial cannot show a half. Ilse wrote both lines side by side so she would stop confusing them.",
    outro="Both figures accepted. The clock takes the whole one and starts; the dial never could show the half.",
    log="Day 51. Two ways to divide. One slash gives you the truth and two slashes give you what the clock can show. I keep mixing them up, so they sit on adjacent lines now.",
    goal_output=["3.5", "3"], unlocks="clock.shift=true", panel="SHIFT CLOCK  split",
))

# 6. the door plate (a1-l06): string ops. The plate printer stamps the cold store's door, and the door releases.
S6 = ensure_script("DA_S1_DoorPlate", "PLATE PRINTER  cold store", """# door plate
# ilse, day 58
# the rule under the name: one character, told how many times.

room = "cold store"
rule = "-" * 12

print(room)
print(rule)
print(len(room))
""", ["print", "len"], dict(
    concepts=["string-ops", "output"],
    predictions=[
        prediction("p1", "What does line 9 stamp onto the plate?", 9, 1, "output", [
            option("a", "------------  (twelve dashes)"),
            option("b", "-12", "type-confusion", "A star between text and a number repeats the text. It does not glue the number on."),
            option("c", "12", "operator-confusion", "The star repeats; it does not count. Twelve dashes."),
            option("d", "-  (one dash)", "loop-runs-once", "Told twelve times, it made twelve. One would have been the rule on its own."),
        ], "a"),
        prediction("p2", "Line 10 measures the room name. What number is that?", 10, 1, "output", [
            option("a", "10"),
            option("b", "9", "fencepost", "The space between the words is a character too. Ten."),
            option("c", "2", "type-confusion", "len counts characters, not words."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'one character, told how many times to be.' And the space in the middle of a name is a character like any other."),
           hint(2, "A star between a piece of text and a number repeats the text that many times. len counts every character in a piece of text, spaces included.", 4)],
    intro="The plate printer takes three lines and stamps them into the enamel. Ilse used it to relabel every door on the west corridor. The rule underneath the name is not typed out; it is one character, told how many times to be. The cold store door stays shut until its plate is right.",
    outro="The plate comes out warm. COLD STORE, and a rule the right length under it. The door releases.",
    log="Day 58. Relabelled the west doors. The printer wants a rule under the name and I was typing the dashes by hand until I remembered the star.",
    goal_output=["cold store", "------------", "10"], unlocks="door.coldstore=true", panel="PLATE  cold store door",
))

# 7. the manifest (a1-l07): type coercion, string concatenation. The crates get their manifest line.
S7 = ensure_script("DA_S1_Manifest", "MANIFEST  outbound", """# outbound manifest
# ilse, day 64
# the manifest is text. a number has to be told to become text first.

crates = 6
count = str(crates)
line = "crates: " + count

print(line)
print(count + count)
""", ["print", "str"], dict(
    concepts=["type-coercion", "string-ops"],
    predictions=[
        prediction("p1", "What does line 9 write on the manifest?", 9, 1, "output", [
            option("a", "crates: 6"),
            option("b", "crates: crates", "type-confusion", "count keeps the text 6, not the word crates."),
            option("c", "crates: ", "sequence-ignored", "count was made on line 6, before line 7 joined it on."),
            option("d", "Nothing, you cannot join text to a number", "type-confusion", "You cannot. That is why line 6 made the number into text first."),
        ], "a"),
        prediction("p2", "Line 10 adds count to itself. What comes out?", 10, 1, "output", [
            option("a", "66"),
            option("b", "12", "type-confusion", "Once it is text, a plus joins. Six next to six."),
            option("c", "6", "operator-confusion", "A plus between two pieces of text does not pick one. It joins them."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'once you have said it, the number stops behaving like a number.' Line 6 is where it is said."),
           hint(2, "str() makes a piece of text out of a number. A plus between two pieces of text joins them end to end; it never adds them up.", 4)],
    intro="Nothing leaves Vantskar without a manifest line, and the manifest is text, all of it, including the count. Ilse's script turns the number into text first and then joins it on. Watch what the number becomes.",
    outro="Manifest accepted. Six crates, in writing.",
    log="Day 64. The manifest is a line of text and a number is not text. You have to say so out loud. Once you have said it, the number stops behaving like a number and I have stopped being surprised by that.",
    goal_output=["crates: 6", "66"], unlocks="manifest.outbound=true", panel="MANIFEST",
))

# 8. the Make task (a1-l08, the beacon, re-themed): write the script that lights the approach to the airlock.
S8 = ensure_script("DA_S1_ApproachLights", "APPROACH RIG  ilse's missing script", """# approach rig
# the rig reads the level, then the word READY on the log.
# nothing else. it is not a clever rig.

level = 9
""", ["print", "set_light"], dict(
    concepts=["variable-assignment", "output", "type-coercion"],
    hints=[hint(1, "Ilse's note from the page before: 'the rig reads two things. the level, then the word.'"),
           hint(2, "set_light(\"approach\", level) sets the approach rig. print() writes one line each time it is called.", 4),
           hint(3, "The level is already under a name on line 5. The word is a piece of text, in quotes, exactly as the rig expects to read it: READY.", 8)],
    intro="Every other script in this sector is hers. This one is not: the folder still lists it and the file is gone, and the date says she deleted it herself, months before she left. The rig wants the level, then the word READY. Two lines. You have read seven scripts to get here; write this one.",
    outro="Level nine, then READY. The approach lights come up the way they did when someone else was writing to them.",
    log="Day 71. Deleted the approach script. Whoever comes after me: it was four lines and I could not make myself leave it behind. Write your own. It will be better, because it will be yours, and because you will have had to know what every line does before you could put it there.",
    tests=[test_case("the rig is set to the level on line 5", world={"light.approach": "9"}),
           test_case("the log reads READY, and nothing else", output=["READY"])],
    free_edit=True, panel="APPROACH RIG", run_cost=8, predicted_run_cost=4,
))

# the corridor lights loop from Phase 3: optional, off the sector's count (loops are Sector 3's concept)
lights_script = ensure_script("DA_Corridor_Lights", "CORRIDOR 1  lights (engineer's terminal)", """# corridor lights
# ilse: the old ballasts pop if you slam them on. bring them
# up a notch at a time and let each notch settle.

level = 0
for notch in range(9):
    level = level + 1
    set_light("corridor", level)
    for tick in range(3):
        wait(1)
log("corridor lit")
""", ["log", "set_light", "wait", "range"], dict(
    tier=3, concepts=["for-loop", "range", "nested-loop", "accumulator"],
    predictions=[prediction("p1", "Line 10, the wait. How many times does it run before the log on line 11?", 11, 1, "count", [
        option("a", "27. Three waits for each of nine notches."),
        option("b", "9. Once per notch; the inner loop runs once.", "loop-runs-once", "The inner loop runs to the end every time the outer one comes round. Twenty-seven."),
        option("c", "30. range(9) counts to nine inclusive, three each.", "fencepost", "range(9) is nought to eight. Nine notches, three each."),
    ], "a")],
    hints=[hint(1, "two loops. the inner one runs to the end every time the outer one comes round."),
           hint(2, "range(9) gives 0 to 8, nine notches. range(3) gives three ticks inside each. count them up.", 3)],
    intro="Corridor lights. She wrote this one later, and gently; the ballasts are older than both of us. It is not on the checklist. It is here because it is hers.",
    outro="Corridor lit, all nine notches. The ballasts held.",
    goal_key="light.corridor", goal_value="9", counts=False, run_cost=12, predicted_run_cost=4,
))
# the Phase 2 door script is retired: its branch is a Sector 2 concept, and the airlock opens on the sector
if EAL.does_asset_exist("/Game/Scripts/DA_Airlock_InnerDoor"):
    EAL.delete_asset("/Game/Scripts/DA_Airlock_InnerDoor")

# --- 3. the level ----------------------------------------------------------------------------
# Sector 1, Habitation: the entry airlock chamber, a main corridor with a doorway segment each
# side, the generator room and the cold store off it, a corner, the leg to the airlock and the
# sealed Sector 2 door beyond. Eight terminals, one per system, each with its panel.
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


# the shell: entry chamber, main run, two doorway segments, the corner, the leg, the beyond
place_mesh("SM_Kit_CorridorSegment", (-400, 0, 0), label="Entry_Chamber")
sealed_door((-415, 0, 0), 0, "EntryAirlock_Outer")
place_mesh("SM_Kit_CorridorSegment", (0, 0, 0), label="Corridor_A")
place_mesh("SM_Kit_CorridorDoorway", (400, 0, 0), label="Corridor_B_DoorwayNorth")
place_mesh("SM_Kit_CorridorDoorway", (1200, 0, 0), rot=(0, 180, 0), label="Corridor_C_DoorwaySouth")
place_mesh("SM_Kit_CorridorCorner", (1200, 0, 0), label="Corner")
place_mesh("SM_Kit_CorridorSegment", (1350, 150, 0), rot=(0, 90, 0), label="Corridor_D_ToAirlock")
place_mesh("SM_Kit_CorridorSegment", (1350, 580, 0), rot=(0, 90, 0), label="Corridor_E_Beyond")
sealed_door((1350, 995, 0), 90, "Sector2_Sealed")
place_mesh("SM_Kit_CorridorSegment", (600, 150, 0), rot=(0, 90, 0), label="Room_Generator")
place_mesh("SM_Kit_WallCap", (600, 550, 0), rot=(0, 90, 0), label="Room_Generator_Cap")
place_mesh("SM_Kit_CorridorSegment", (1000, -150, 0), rot=(0, -90, 0), label="Room_ColdStore")
place_mesh("SM_Kit_WallCap", (1000, -550, 0), rot=(0, -90, 0), label="Room_ColdStore_Cap")

# doors the scripts release: the entry interlock, the cold store's door, the airlock on the sector
gits_door((-15, 0, 0), 0, "entry", "Door_Entry")
gits_door((1000, -160, 0), 90, "coldstore", "Door_ColdStore", with_frame=False)
gits_door((1350, 565, 0), 90, "inner", "Door_Airlock")

# 1. door log, in the entry chamber
terminal((-200, 128, 0), 90, S1, "T1_DoorLog")
panel((-320, 149, 150), 90, S1, "P1_DoorLog")
# 2. gauge mirror, corridor A, the pressure display
terminal((320, 128, 0), 90, S2, "T2_GaugeMirror")
panel((150, 149, 150), 90, S2, "P2_Pressure")
# 5. shift clock, corridor B south wall
terminal((500, -128, 0), -90, S5, "T5_ShiftClock")
panel((640, -149, 150), -90, S5, "P5_ShiftClock")
clock = spawn(unreal.GitsClock, (760, -149, 175), rot=(0, -90, 0), label="Clock_Shift")
clock.body.set_static_mesh(mesh("SM_Kit_WallGauge"))
clock.set_editor_property("system_id", "shift")
clock.set_actor_scale3d(unreal.Vector(0.5, 0.5, 0.5))
# 4. reclaimer panel and the vent, corridor C north wall
terminal((880, 128, 0), 90, S4, "T4_Reclaimer")
panel((1010, 149, 150), 90, S4, "P4_Reclaimer")
vent = spawn(unreal.GitsVent, (1140, 149, 150), rot=(0, 90, 0), label="Vent_Reclaimer")
vent.grille.set_static_mesh(mesh("SM_Kit_Vent"))
vent.fan.set_static_mesh(mesh("SM_Kit_VentFan"))
vent.set_editor_property("system_id", "reclaimer")
# 6. the door plate, corridor C south wall by the cold store door, with the plate above the door
terminal((1120, -128, 0), -90, S6, "T6_DoorPlate")
panel((1000, -149, 232), -90, S6, "P6_Plate", scale=0.4)
# 3. cold store setpoint and 7. the manifest, inside the cold store
terminal((1130, -400, 0), 0, S3, "T3_ColdStore")
panel((1149, -280, 150), 0, S3, "P3_ColdStore")
terminal((870, -300, 0), 180, S7, "T7_Manifest")
panel((851, -440, 150), 180, S7, "P7_Manifest")
# 8. the Make task on the leg to the airlock
terminal((1480, 250, 0), 0, S8, "T8_ApproachRig")
panel((1494, 400, 150), 0, S8, "P8_ApproachRig")
# the engineer's terminal, optional, in the generator room
terminal((470, 400, 0), 180, lights_script, "T9_CorridorLights_Optional")

gauge = spawn(unreal.GitsPowerGauge, (150, -149, 170), rot=(0, -90, 0), label="PowerGauge")
gauge.body.set_static_mesh(mesh("SM_Kit_WallGauge"))
gauge.set_actor_scale3d(unreal.Vector(0.5, 0.5, 0.5))

generator = spawn(unreal.GitsGenerator, (600, 520, 0), rot=(0, 90, 0), label="Generator")
generator.body.set_static_mesh(mesh("SM_Kit_Generator"))

# dressing: the cold store unit, crates, and the drone that will fly in Sector 3, parked
place_mesh("SM_Kit_ColdStoreUnit", (1000, -515, 0), rot=(0, -90, 0), label="ColdStoreUnit")
place_mesh("SM_Kit_Crate", (890, -500, 0), rot=(0, 8, 0), label="Crate_1")
place_mesh("SM_Kit_Crate", (890, -500, 60), rot=(0, 20, 0), label="Crate_2")
place_mesh("SM_Kit_Crate", (1110, -500, 0), rot=(0, 3, 0), label="Crate_3")
place_mesh("SM_Kit_Drone", (700, 330, 0), rot=(0, 25, 0), label="Drone_Parked")

station = spawn(unreal.GitsStation, (0, 0, 0), label="Station")
spec = unreal.GitsSensorSpec()
spec.set_editor_property("id", "airlock"); spec.set_editor_property("value", 4.0); spec.set_editor_property("drift_per_tick", 0.0)
station.set_editor_property("sensors", [spec])
station.set_editor_property("initial_switches", {"door.entry": False, "door.coldstore": False, "door.inner": False, "vent.reclaimer": False, "clock.shift": False})
station.set_editor_property("initial_levels", {"light.coldstore": 0.0, "light.approach": 1.0})
station.set_editor_property("power_budget", 60)
station.set_editor_property("reserve_restore", 30)
station.set_editor_property("sector_complete_line", "Eight systems. Every one of them was hers and every one of them runs. Airlock to Sector 2 released; the greenhouse is through there, and I have not been able to look at it since she left.")
station.set_editor_property("sector_unlocks", "door.inner=true")


def ceiling_light(loc, system_id, level, label, full=12.0):
    light = spawn(unreal.GitsLight, loc, label=label)
    light.fitting.set_static_mesh(mesh("SM_Kit_CeilingLight"))
    light.set_editor_property("system_id", system_id)
    light.set_editor_property("initial_level", level)
    light.set_editor_property("full_intensity", full)
    return light


ceiling_light((-200, 0, 299), "entry", 3.0, "Light_Entry")
for i, loc in enumerate(((200, 0, 299), (600, 0, 299), (1000, 0, 299), (1350, 0, 299))):
    ceiling_light(loc, "corridor", 2.5, f"Light_Corridor_{i+1}")
ceiling_light((1350, 250, 299), "approach", 1.0, "Light_Approach_1")
ceiling_light((1350, 450, 299), "approach", 1.0, "Light_Approach_2")
ceiling_light((600, 380, 299), "hab", 4.5, "Light_GeneratorRoom")
ceiling_light((1000, -380, 299), "cold", 2.5, "Light_ColdStore")
ceiling_light((1350, 780, 299), "beyond", 1.0, "Light_Beyond")

# the cold store's frost light: comes on when the setpoint reads true (light.coldstore, 0 to 10)
frost = spawn(unreal.GitsLight, (1000, -470, 120), label="Light_ColdStoreFrost")
frost.set_editor_property("system_id", "coldstore")
frost.set_editor_property("initial_level", 0.0)
frost.set_editor_property("full_intensity", 6.0)
frost.light.set_light_color(unreal.LinearColor(0.45, 0.7, 1.0, 1.0))

for i, loc in enumerate(((400, 0, 285), (1350, 200, 285), (1000, -300, 285), (-200, 0, 285))):
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


cold_fill((1000, -420, 200), 1.2, 500.0, "Fill_ColdStore")
cold_fill((1350, 900, 140), 1.2, 500.0, "Fill_Beyond")
cold_fill((-340, -110, 40), 0.8, 400.0, "Fill_Entry")

ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess")
ppv.set_editor_property("unbound", True)
settings = ppv.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 0.0)
settings.set_editor_property("auto_exposure_max_brightness", 0.0)
ppv.set_editor_property("settings", settings)

start = spawn(unreal.PlayerStart, (-250, 0, 100), rot=(0, 0, 0), label="PlayerStart")

LES.save_current_level()
print("level saved:", LEVEL, "| actors:", len(EAS.get_all_level_actors()))
