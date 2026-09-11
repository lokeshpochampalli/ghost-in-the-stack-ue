# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/setup_sector2_content.py).
# Sector 2, the Greenhouse: eight systems re-authored from the reference's Act 2 (a2-l01 to
# a2-l08), conditionals and comparison, and the level they live in. Safe to re-run.
import sys, os
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__))))
from gits_content_lib import *

ACT = 2
LEVEL = "/Game/Sectors/L_Sector2_Greenhouse"
NEXT = "/Game/Sectors/L_Sector3_Logistics"

# --- the scripts ---------------------------------------------------------------------------

S1 = ensure_script("DA_S2_FrostAlarm", "GREENHOUSE  frost alarm", """# frost alarm
# ilse, day 96
# below the line it says so. above it, it says the other thing.

outside = -19
limit = -15

if outside < limit:
    print("FROST")
else:
    print("CLEAR")
""", ["print"], dict(
    level_id="a2-l01", act=ACT, tier=2,
    concepts=["conditional", "comparison", "output"],
    predictions=[prediction("p1", "Which of the two lines does the alarm take?", 9, 1, "branch", [
        option("a", "FROST"),
        option("b", "CLEAR", "inverted-comparison", "Minus nineteen is the colder. Below the limit means less than it, and the alarm says so."),
        option("c", "FROST, and then CLEAR", "branch-both", "One question, one branch. The else is the road not taken, and it is not taken."),
    ], "a")],
    hints=[hint(1, "Ilse wrote the rule at the top of her own script. Minus nineteen and minus fifteen: which of those is the colder?"),
           hint(2, "The station asks the question on line 8 exactly once and then takes one branch. The other branch is not run at all, and nothing goes back to check it.", 4)],
    intro="Winter closed while you were reading her surface scripts. From here on the station has to decide things without you. Ilse wrote the first of those on her ninety-sixth day, and it is four lines that ask one question. Read the question before you run it.",
    outro="FROST, and the alarm holds. The corridor heaters wind up somewhere below you.",
    log="Day 96. First script here that makes up its own mind. It compares two numbers and takes one of two lines depending on the answer, and only one of them, which took me longer to believe than it should have. I keep expecting both to happen.",
    goal_output=["FROST"], unlocks="light.frost=10", panel="FROST ALARM", run_cost=12, predicted_run_cost=4,
))

S2 = ensure_script("DA_S2_HeaterLadder", "HEATERS  the ladder", """# heater ladder
# ilse, day 108. the first rung that fits is the one it takes. it stops there.

wind = 22

if wind > 30:
    setting = 3
elif wind > 20:
    setting = 2
elif wind > 10:
    setting = 1
else:
    setting = 0

print(setting)
""", ["print"], dict(
    level_id="a2-l02", act=ACT, tier=2,
    concepts=["elif-chain", "comparison", "conditional"],
    predictions=[
        prediction("p1", "What setting does line 15 report?", 15, 1, "output", [
            option("a", "2"),
            option("b", "1", "branch-both", "The ladder stops at the first rung that fits. Twenty-two is over twenty, so it stops at two and never asks about ten."),
            option("c", "3", "inverted-comparison", "Twenty-two is not over thirty. The first rung does not fit; the second does."),
            option("d", "0", "sequence-ignored", "The else is only reached when no rung fits. One did."),
        ], "a"),
        prediction("p2", "Line 9 is the rung that fits. What happens to the two rungs below it?", 9, 1, "branch", [
            option("a", "Nothing. The ladder stops here."),
            option("b", "They are asked too, and the last one that fits wins.", "branch-both", "That is four separate ifs, and it is the mistake she made for two days. An elif ladder stops at the first fit."),
            option("c", "They are asked, and the else at the bottom puts it back to 0.", "sequence-ignored", "The else belongs to the ladder. Once a rung fits, nothing below it runs, the else included."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse's rule is on line 2, and her log entry says what it cost her to learn it: the first rung that fits, not the last."),
           hint(2, "Read the rungs top to bottom and stop at the first one whose question is true. Everything below it, the else included, is skipped entirely.", 5)],
    intro="Four settings, one wind speed, and a ladder of questions that stops at the first rung it can stand on. The heaters draw hard, so the station will not let you run this twice on a guess.",
    outro="Setting two. The ducts change note and the corridor stops whistling.",
    log="Day 108. Built the heater a ladder instead of four separate questions. The difference matters: with four separate ifs it keeps asking after it has already found the answer, and the last one that fits wins instead of the first. I had it wrong for two days and the heaters were running at nothing.",
    goal_output=["2"], unlocks="heater.corridor=2", panel="HEATERS  setting", run_cost=14, predicted_run_cost=5,
))

S3 = ensure_script("DA_S2_Interlock", "AIRLOCK 2  interlock", """# airlock interlock
# ilse, day 121. and means both. one of them is not enough.

inner_shut = True
outer_shut = False

if inner_shut and outer_shut:
    print("SEALED")
else:
    print("OPEN TO THE OUTSIDE")
""", ["print"], dict(
    level_id="a2-l03", act=ACT, tier=2, editable_lines=[4, 5],
    concepts=["boolean-logic", "conditional", "data-types"],
    predictions=[prediction("p1", "What does the interlock stamp on the panel?", 10, 1, "branch", [
        option("a", "OPEN TO THE OUTSIDE"),
        option("b", "SEALED", "operator-confusion", "and means both. True and False is a no, so the interlock takes the else."),
        option("c", "SEALED, and then OPEN TO THE OUTSIDE", "branch-both", "One question. One branch. The other line is never reached."),
    ], "a")],
    hints=[hint(1, "Ilse: 'and means both. one of them is not enough.' One of the two readings on lines 4 and 5 is not a yes."),
           hint(2, "A question joined by and is only true when both halves are true. True and False is not a yes, so the station takes the other branch.", 4)],
    intro="The airlock will not report itself sealed unless both doors agree, and one of them does not. Ilse's interlock asks a single question with two halves. You may change the two readings on lines 4 and 5, but read the question first. It is not the question most people think it is.",
    outro="OPEN TO THE OUTSIDE, in enamel letters, which is the station being honest with you.",
    log="Day 121. and means both. One of them is not enough. I have the paperwork to prove it and a report to the depot that nobody read.",
    goal_output=["OPEN TO THE OUTSIDE"], unlocks="light.interlock=10", panel="INTERLOCK", run_cost=12, predicted_run_cost=4,
))

S4 = ensure_script("DA_S2_EmptyChannel", "SURVEY  channel check", """# channel check
# ilse, day 134. an empty reading is a no. zero is a no.

label = ""
depth = 0
note = "shelf 2"

if label:
    print("LABEL")

if not depth:
    print("NO DEPTH")

if note:
    print("NOTE")
""", ["print"], dict(
    level_id="a2-l04", act=ACT, tier=2, editable_lines=[4, 5, 6],
    concepts=["truthiness", "conditional", "data-types"],
    predictions=[
        prediction("p1", "Line 8 asks about label, which is empty. Which way does it go?", 8, 1, "branch", [
            option("a", "Past it. Nothing is printed."),
            option("b", "Into it. LABEL is printed.", "type-confusion", "An empty piece of text counts as a no. The question is asked of the text itself, and the text has nothing in it."),
            option("c", "Into it and past it both.", "branch-both", "A question goes one way. This one goes past."),
        ], "a"),
        prediction("p2", "How many of the three checks print anything at all?", 15, 1, "count", [
            option("a", "2"),
            option("b", "3", "type-confusion", "The empty label is a no. Two of the three print."),
            option("c", "1", "operator-confusion", "not turns zero's no into a yes, so NO DEPTH prints; and 'shelf 2' is a yes on its own. Two."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'an empty reading is a no. zero is a no.' Line 11 has a not in front of its question, which turns the answer over."),
           hint(2, "An empty piece of text and the number zero both count as no. Anything else counts as yes. not turns a no into a yes and a yes into a no.", 5)],
    intro="Three channels on the survey panel, and the panel treats an empty one as a no without being asked to. Ilse found the same rule inside the language and stopped writing the comparison out. Three questions here, and not all of them are answered the way they look.",
    outro="NO DEPTH and NOTE. The panel accepts two lines and asks nothing about the third. Water goes down the channel that has something in it.",
    log="Day 134. An empty piece of text is a no. Zero is a no. I stopped writing == 0 everywhere and the scripts got shorter and, I think, more honest. The panel has always worked this way and now the script says so out loud.",
    goal_output=["NO DEPTH", "NOTE"], unlocks="valve.channel=true", panel="CHANNEL CHECK", run_cost=14, predicted_run_cost=5,
))

S5 = ensure_script("DA_S2_Fallback", "LOG  the name line", """# name for the log line
# ilse, day 146. or hands back the thing itself, not a yes or a no.

given = ""
fallback = "VANTSKAR"

name = given or fallback

print(name)
print(given or 0 or "unnamed")
""", ["print"], dict(
    level_id="a2-l05", act=ACT, tier=2, editable_lines=[4, 5],
    concepts=["boolean-logic", "truthiness", "data-types", "string-ops"],
    predictions=[
        prediction("p1", "What does line 9 write into the log?", 9, 1, "output", [
            option("a", "VANTSKAR"),
            option("b", "True", "type-confusion", "or does not hand back a yes. It hands back the first thing that counts as one: the fallback, the text itself."),
            option("c", "Nothing. An empty line.", "operator-confusion", "The empty given is a no, so or moves on to the fallback and hands that back."),
        ], "a"),
        prediction("p2", "Line 10 chains three of them together. What comes out?", 10, 1, "output", [
            option("a", "unnamed"),
            option("b", "True", "type-confusion", "The thing itself, never a yes or a no. Empty text is a no, zero is a no, so the last thing comes back."),
            option("c", "0", "operator-confusion", "Zero is a no, so or keeps walking. The last one is what comes back when nothing before it counts."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'not a yes or a no. the thing itself.' An empty piece of text is a no, and so is zero."),
           hint(2, "or walks left to right and stops at the first thing that counts as a yes, handing that thing back. If every one of them is a no, it hands back the last.", 5)],
    intro="Every line in the station log carries the name of the place that wrote it, and one of the recorders came back from the depot with its name field wiped. Ilse's fix is a single word. What that word hands back is not what most people expect it to hand back.",
    outro="VANTSKAR, on every line, in the same place it always was. The nameplate over the recorder comes on.",
    log="Day 146. or does not hand you back a yes or a no. It hands you back the thing itself: the first one that counts as a yes, or the last one if none of them do. I read that three times before I believed it and then I used it everywhere.",
    goal_output=["VANTSKAR", "unnamed"], unlocks="light.nameplate=10", panel="RECORDER  name", run_cost=14, predicted_run_cost=5,
))

S6 = ensure_script("DA_S2_IceLine", "SHELF  the alarm", """# shelf alarm
# ilse, day 161
# threshold was 40. then 55. it is 70 now and i am not proud of that.

drift = 68

if drift > 70:
    state = "ALARM"
elif drift > 55:
    state = "WATCH"
else:
    state = "NORMAL"

print(state)
""", ["print"], dict(
    level_id="a2-l06", act=ACT, tier=2, editable_lines=[5, 6, 7, 8, 9, 10, 11, 12],
    concepts=["elif-chain", "comparison", "conditional", "reassignment"],
    predictions=[prediction("p1", "Which state does line 14 report?", 14, 1, "output", [
        option("a", "WATCH"),
        option("b", "ALARM", "inverted-comparison", "Sixty-eight is not over seventy. The first rung does not fit."),
        option("c", "NORMAL", "branch-both", "The second rung fits, and the ladder stops there. The else is never reached."),
    ], "a")],
    hints=[hint(1, "Sixty-eight against seventy, and then sixty-eight against fifty-five. The ladder asks them in that order and stops when one of them fits."),
           hint(2, "The first question is false, so the station moves down to the second. The second is true, so the else at the bottom is never reached.", 5),
           hint(3, "You can edit these lines. If you put the threshold back to forty, sixty-eight clears it and the panel says something else entirely, which is the thing she stopped it saying.", 8)],
    intro="The shelf alarm has three states and one number to decide between them. Read the comment at the top before you read the code. Ilse left the whole history of this script in one line, and she is not proud of it. You can change the numbers here. Whether you should is a different question.",
    outro="WATCH. The panel goes amber and stays amber, which is what she set it up to do.",
    log="Day 161. Raised the shelf threshold to seventy. It was forty when I arrived and I have moved it twice, and both times the reason I gave myself was that the sensor must be drifting. The sensor is not drifting. I know what is happening and I have raised the number instead of writing it down. That is what this entry is for.",
    goal_output=["WATCH"], unlocks="light.shelf=10", panel="SHELF ALARM", run_cost=14, predicted_run_cost=5,
))

S7 = ensure_script("DA_S2_NightWatch", "WEST CORRIDOR  night watch", """# night watch
# ilse, day 178. the inner question only gets asked if the outer one says yes.

hour = 3
outside = -31

if hour < 6:
    if outside < -30:
        print("HOLD")
    else:
        print("STAND DOWN")
else:
    print("DAY")
""", ["print"], dict(
    level_id="a2-l07", act=ACT, tier=2, editable_lines=[4, 5],
    concepts=["conditional", "comparison", "boolean-logic"],
    predictions=[
        prediction("p1", "What does the night watch print?", 9, 1, "branch", [
            option("a", "HOLD"),
            option("b", "STAND DOWN", "inverted-comparison", "Minus thirty-one is the colder. It is less than minus thirty, so the inner question says yes."),
            option("c", "HOLD, and then DAY", "branch-both", "The else at the bottom belongs to the outer question, which said yes. It is skipped."),
        ], "a"),
        prediction("p2", "Line 8 compares minus thirty-one with minus thirty. Which way does it go?", 8, 1, "branch", [
            option("a", "Into the HOLD line. Minus thirty-one is the colder of the two."),
            option("b", "Into the STAND DOWN line.", "inverted-comparison", "Colder numbers are further down. Minus thirty-one is less than minus thirty."),
            option("c", "Neither. The station will not compare negative numbers.", "type-confusion", "A number is a number. The station compares them all the same way."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'the inner question only gets asked if the outer one says yes.' Three is before six, so it is asked."),
           hint(2, "Colder numbers are further down, so minus thirty-one is less than minus thirty. Once the inner question is answered, the outer else at the bottom belongs to the outer question and is skipped.", 5)],
    intro="Two questions, one nested inside the other, and the inner one is only asked if the outer one says yes. Three in the morning and thirty-one below. The heaters cannot hold the whole corridor at that temperature, so the script decides which half to give up.",
    outro="HOLD. The far end of the west corridor goes dark and cold, and the rooms you are standing in stay warm.",
    log="Day 178. Nested the night watch. The indenting is the whole meaning: the second question sits inside the first, so on a warm night it is never asked at all. I like that the shape of the thing on the page is the shape of the thing it does.",
    goal_output=["HOLD"], unlocks="light.west=0", panel="NIGHT WATCH", run_cost=14, predicted_run_cost=5,
))

S8 = ensure_script("DA_S2_CorridorLamp", "WEST CORRIDOR  the lamp", """# corridor lamp
# ilse, day 185
# one word per reading: ON or OFF. on when the light is below the threshold,
# off when it is above it, and off when it is exactly on it.
# three readings, in this order: now, dusk, noon.

threshold = 30
now = 12
dusk = 30
noon = 480
""", ["print", "open_valve", "close_valve"], dict(
    level_id="a2-l08", act=ACT, tier=2, free_edit=True,
    concepts=["conditional", "comparison", "output"],
    tests=[test_case("the lamp is told what to do for each of the three readings, in that order", output=["ON", "OFF", "OFF"]),
           test_case("and the lamp itself is switched to match the reading now", world={"valve.lamp": "true"})],
    hints=[hint(1, "Ilse's rule is in her comment, including the part that cost her a fortnight: exactly on the threshold is off, not on."),
           hint(2, "Each reading needs its own question and its own two branches. Three readings, three questions, and one word printed by each.", 5),
           hint(3, "The lamp fitting itself is switched with open_valve(\"lamp\") and close_valve(\"lamp\"). Only the reading called now decides which of those the corridor gets.", 10)],
    intro="The lamp over the west corridor has no rule any more. Ilse took hers out on the day she wrote the note below and left the readings behind, which is either an oversight or an invitation. Three readings, one word each, and the lamp switch itself for the reading now. You have read seven scripts that decide things. Write one.",
    outro="ON, OFF, OFF, and the lamp comes up over the west corridor for the first time in eleven months.",
    log="Day 185. Took my rule out of the corridor lamp. It was one line and it was wrong at exactly the threshold, and rather than fix it I have left the readings and the wiring and a note. Whoever is reading this can decide what dark means here. I have stopped being sure.",
    panel="CORRIDOR LAMP", run_cost=10, predicted_run_cost=5,
))

# --- the level -----------------------------------------------------------------------------
# The Greenhouse: an entry chamber off Sector 1's airlock, a long grow corridor with planters
# and grow lights, the heater room north, the channel room and the west corridor south, a
# corner and the leg to the drone bay, where the Sector 3 gate is.
begin_level(LEVEL)

place_mesh("SM_Kit_CorridorSegment", (-400, 0, 0), label="Entry_Chamber")
sealed_door((-415, 0, 0), 0, "Sector1_Airlock")
place_mesh("SM_Kit_CorridorSegment", (0, 0, 0), label="Corridor_A")
place_mesh("SM_Kit_CorridorDoorway", (400, 0, 0), label="Corridor_B_DoorwayNorth")
place_mesh("SM_Kit_CorridorSegment", (800, 0, 0), label="Corridor_C")
place_mesh("SM_Kit_CorridorDoorway", (1200, 0, 0), rot=(0, 180, 0), label="Corridor_D_DoorwaySouth")
place_mesh("SM_Kit_CorridorCorner", (1600, 0, 0), label="Corner")
place_mesh("SM_Kit_CorridorSegment", (1750, 150, 0), rot=(0, 90, 0), label="Corridor_E_Leg")
place_mesh("SM_Kit_CorridorSegment", (1750, 550, 0), rot=(0, 90, 0), label="Corridor_F_DroneBay")
room_off(600, 0, +1, "Room_Heaters")
room_off(1400, 0, -1, "Room_Channel")

# the entry interlock opens on the frost alarm; the drone bay gate opens on the sector
gits_door((-15, 0, 0), 0, "entry", "Door_Entry")
sector_gate((1750, 995, 0), 90, "door.dronebay", NEXT, "SECTOR 3  LOGISTICS", "Gate_Sector3",
            "Sealed. The drone bay opens when the greenhouse is alive: every one of her deciding scripts, read and run.")

# 1. the frost alarm, in the entry chamber, with its amber lamp
terminal((-200, 128, 0), 90, S1, "T1_FrostAlarm")
panel((-320, 149, 150), 90, S1, "P1_FrostAlarm")
lamp((-200, 0, 280), "frost", "Lamp_Frost", full=8.0, color=(1.0, 0.62, 0.15, 1.0))
# 3. the interlock, corridor A south wall, with a red lamp over the outer hatch
terminal((200, -128, 0), -90, S3, "T3_Interlock")
panel((80, -149, 150), -90, S3, "P3_Interlock")
lamp((-380, 0, 270), "interlock", "Lamp_Interlock", full=6.0, color=(1.0, 0.2, 0.12, 1.0))
# 2. the heater ladder, in the heater room, with the corridor heater on the corridor wall
terminal((470, 400, 0), 180, S2, "T2_HeaterLadder")
panel((451, 300, 150), 180, S2, "P2_Heaters")
heater((730, 149, 0), 90, "corridor", "Heater_Corridor")
heater((749, 480, 0), 0, "corridor", "Heater_Room")
# 5. the fallback, corridor C north wall, the recorder's nameplate lit on completion
terminal((900, 128, 0), 90, S5, "T5_Fallback")
panel((1030, 149, 150), 90, S5, "P5_Recorder")
lamp((1030, 120, 240), "nameplate", "Lamp_Nameplate", full=5.0, color=(0.95, 0.9, 0.75, 1.0))
# 6. the ice line, corridor C south wall, the panel goes amber
terminal((1100, -128, 0), -90, S6, "T6_IceLine")
panel((960, -149, 150), -90, S6, "P6_ShelfAlarm")
lamp((960, -120, 240), "shelf", "Lamp_Shelf", full=7.0, color=(1.0, 0.62, 0.15, 1.0))
# 4. the channel check, in the channel room, with the sprinklers over the planters
terminal((1530, -400, 0), 0, S4, "T4_ChannelCheck")
panel((1549, -280, 150), 0, S4, "P4_ChannelCheck")
place_mesh("SM_Kit_Planter", (1400, -350, 0), rot=(0, 90, 0), label="Planter_Channel_1")
place_mesh("SM_Kit_Planter", (1330, -480, 0), rot=(0, 90, 0), label="Planter_Channel_2")
sprinkler((1400, -350, 299), "channel", "Sprinkler_Channel_1")
sprinkler((1330, -480, 299), "channel", "Sprinkler_Channel_2")
# 7. the night watch: the west corridor is the far end of the channel room; its light goes out
terminal((1270, -300, 0), 180, S7, "T7_NightWatch")
panel((1251, -440, 150), 180, S7, "P7_NightWatch")
ceiling_light((1400, -480, 299), "west", 3.0, "Light_West", full=10.0)
# 8. the Make task on the leg to the drone bay, with the lamp it switches
terminal((1880, 250, 0), 0, S8, "T8_CorridorLamp")
panel((1894, 400, 150), 0, S8, "P8_CorridorLamp")
west_lamp = ceiling_light((1750, 700, 299), "lamp", 0.0, "Light_CorridorLamp", full=12.0)
west_lamp.set_editor_property("switch_key", "valve.lamp")

# the grow corridor: planters both sides and grow lights over them
for i, x in enumerate((100, 300, 900, 1100)):
    place_mesh("SM_Kit_Planter", (x, 100, 0), label=f"Planter_N_{i+1}")
    place_mesh("SM_Kit_Planter", (x, -100, 0), label=f"Planter_S_{i+1}")
for i, x in enumerate((200, 1000)):
    ceiling_light((x, 100, 299), "grow", 5.0, f"GrowLight_N_{i+1}", full=6.0, color=(0.95, 0.55, 0.85, 1.0), fitting="SM_Kit_GrowLight")
    ceiling_light((x, -100, 299), "grow", 5.0, f"GrowLight_S_{i+1}", full=6.0, color=(0.95, 0.55, 0.85, 1.0), fitting="SM_Kit_GrowLight")

# house lights, emergency lights, fills
ceiling_light((-200, 0, 299), "entry", 3.0, "Light_Entry")
for i, loc in enumerate(((600, 0, 299), (1400, 0, 299), (1750, 150, 299), (1750, 550, 299))):
    ceiling_light(loc, "corridor", 2.5, f"Light_Corridor_{i+1}")
ceiling_light((600, 380, 299), "heaters", 3.5, "Light_HeaterRoom")
ceiling_light((1400, -250, 299), "channel", 2.5, "Light_ChannelRoom")
emergency_lights([(400, 0, 285), (1200, 0, 285), (1750, 300, 285), (600, 400, 285), (1400, -400, 285)])
fill_light((1400, -450, 200), 1.0, 500.0, "Fill_Channel", color=(0.5, 0.75, 0.6, 1.0))
fill_light((-340, -110, 40), 0.8, 400.0, "Fill_Entry")
fill_light((1750, 900, 140), 1.2, 500.0, "Fill_DroneBay")

wall_gauge((150, -149, 170), -90, "PowerGauge")
generator((600, 520, 0), 90)
place_mesh("SM_Kit_Drone", (1880, 800, 0), rot=(0, -40, 0), label="Drone_Bay_Parked")
place_mesh("SM_Kit_Crate", (1620, 850, 0), rot=(0, 12, 0), label="Crate_Bay")

station({"door.entry": False, "door.dronebay": False, "valve.channel": False, "valve.lamp": False},
        {"light.frost": 0.0, "light.interlock": 0.0, "light.nameplate": 0.0, "light.shelf": 0.0, "light.west": 3.0, "heater.corridor": 0.0, "light.lamp": 0.0},
        90, 45,
        "Eight scripts that decide things, and every one of them decided the way she wrote it. The greenhouse is alive again; the drone bay is released. Logistics is through there, and it is where she kept the survey book.",
        "door.dronebay=true")
exposure_and_start((-250, 0, 100))
save_level(LEVEL)
