# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/setup_sector4_content.py).
# Sector 4, the Reactor: six systems re-authored from the reference's Act 4 (a4-l01 to a4-l06),
# functions, scope and recursion, and what happened to Ilse. Safe to re-run.
import sys, os
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__))))
from gits_content_lib import *

ACT = 4
LEVEL = "/Game/Sectors/L_Sector4_Reactor"

S1 = ensure_script("DA_S4_Conversion", "DEPOT FORM  paces", """# paces for the depot form
# ilse, day 281. two paces to the metre, on flat ice, with my legs.

def paces(metres):
    return metres * 2

drift = 224

print(paces(drift))
print(paces(10))
""", ["print"], dict(
    level_id="a4-l01", act=ACT, tier=4, editable_lines=[7],
    concepts=["function-def", "parameters", "return-value"],
    predictions=[
        prediction("p1", "What does line 9 print?", 9, 1, "value", [
            option("a", "448"),
            option("b", "224", "arg-param-identity", "metres is the number handed over, called by a new name inside. The work doubles it and hands back 448."),
            option("c", "None", "return-vs-print", "return hands the answer back out to the print that asked. It is not nothing."),
            option("d", "226", "operator-confusion", "Times two, not plus two."),
        ], "a"),
        prediction("p2", "And line 10, which hands it a different number?", 10, 1, "value", [
            option("a", "20"),
            option("b", "None", "return-vs-print", "Each call hands its own answer back. Ten doubled is twenty."),
            option("c", "448", "sequence-ignored", "The room opens fresh for every call, with the number it was given this time."),
        ], "a"),
    ],
    hints=[hint(1, "Her log entry says it: 'you hand it a number and it hands one back.' The name in the brackets on line 4 is what the number is called once it is inside."),
           hint(2, "return is what hands the answer back out. The room opens, does its one line of work, hands the answer over and shuts, and it opens fresh for the second call with a different number in it.", 5)],
    intro="Autumn. From here her scripts stop being lists of instructions and start being pieces of work with names on them. This is the first one: a small job she got tired of writing out, given a name so she could ask for it instead.",
    outro="Four hundred and forty-eight paces, and twenty. The depot form takes both without complaint.",
    log="Day 281. Gave a name to a thing I was writing out four times a day. You hand it a number and it hands one back, and the number you hand it does not have to be the same one twice. I do not know why this took me until my ninth month.",
    goal_output=["448", "20"], unlocks="light.depot=10", panel="DEPOT FORM", run_cost=16, predicted_run_cost=5,
))

S2 = ensure_script("DA_S4_TheRooms", "TALLY  the rooms", """# tally
# ilse, day 293. a name made inside the work stays inside it.

count = 0

def tally(items):
    count = items + 1
    return count

print(tally(4))
print(count)
""", ["print"], dict(
    level_id="a4-l02", act=ACT, tier=4, editable_lines=[4],
    concepts=["local-scope", "parameters", "return-value", "function-def"],
    predictions=[
        prediction("p1", "What does line 10 print?", 10, 1, "value", [
            option("a", "5"),
            option("b", "4", "arg-param-identity", "items is four inside the room, and the room adds one before handing it back."),
            option("c", "None", "return-vs-print", "return hands five back out to the print."),
        ], "a"),
        prediction("p2", "And line 11, which asks the outer count what it is holding?", 11, 1, "value", [
            option("a", "0"),
            option("b", "5", "scope-leak", "The count on line 7 is made inside the room and stays there. The outer count was never touched."),
            option("c", "None", "return-vs-print", "The outer count is a name with a value: nought, since line 4."),
            option("d", "4", "arg-param-identity", "Four was the number handed in. The outer count never saw it."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'a name made inside the work stays inside it.' Line 7 makes one. It does not reach back out to line 4."),
           hint(2, "Rewind to the moment line 7 runs and look at the recorder. There is a count in the inner room and a count in the outer one, and only the inner one changes.", 5)],
    intro="There is a name on line 4 and a name on line 7 and they are spelled the same. They are not the same name. Ilse lost an afternoon to this and wrote the afternoon down so that you would not have to lose one as well.",
    outro="Five, and then nought. Both of those are true at the same time, which is the thing worth taking away.",
    log="Day 293. A name made inside a piece of work stays inside it. It does not come out with the answer. Only the thing you hand back comes out. I spent an afternoon watching a count stay at nought while the script that changed it worked perfectly, and the script was right and I was wrong.",
    goal_output=["5", "0"], unlocks="light.tally=10", panel="TALLY", run_cost=16, predicted_run_cost=5,
))

S3 = ensure_script("DA_S4_TwoRooms", "REPORT  two rooms", """# report
# ilse, day 305. the inner room shuts before the outer one is finished with it.

def metres(paces):
    return paces // 2

def report(paces):
    walked = metres(paces)
    return walked + 1

print(report(9))
print(metres(9))
""", ["print"], dict(
    level_id="a4-l03", act=ACT, tier=4, editable_lines=[11, 12],
    concepts=["call-stack", "parameters", "local-scope", "return-value", "arithmetic"],
    predictions=[
        prediction("p1", "What does line 11 print?", 11, 1, "value", [
            option("a", "5"),
            option("b", "4.5", "type-confusion", "Two slashes throw the remainder away. Nine halved that way is four, and report adds one."),
            option("c", "10", "sequence-ignored", "Line 8 has to finish before line 9 can start. metres hands back four; then one is added."),
        ], "a"),
        prediction("p2", "And line 12, which calls the inner one directly?", 12, 1, "value", [
            option("a", "4"),
            option("b", "4.5", "type-confusion", "Whole division. Four, remainder thrown away."),
            option("c", "5", "fencepost", "Five was report's answer, which added one. metres alone is four."),
        ], "a"),
        prediction("p3", "Inside metres, where does its paces get its value from?", 5, 1, "value", [
            option("a", "It is a copy of the number report handed over."),
            option("b", "It is report's paces. The same name, so the two move together.", "arg-param-identity", "Same spelling, different room. metres gets a copy of the number and nothing else."),
            option("c", "It is empty. metres has its own paces and nothing has filled it.", "scope-leak", "Handing a number over is what fills it. It arrives with the call."),
        ], "a"),
    ],
    hints=[hint(1, "Two slashes throw away the remainder, as they did at the mixer. Nine halved that way is not four and a half."),
           hint(2, "Line 8 has to finish before line 9 can start, because line 9 needs what line 8 handed back. Rewind to the inner room and read what is on its shelf.", 6)],
    intro="One piece of work that calls another. The inner room opens inside the outer one and shuts before the outer one has finished with it, and the recorder will show you both at once if you rewind to the right moment. Two names spelled paces here, in two different rooms.",
    outro="Five, and four. The same job done twice with one step of difference between them.",
    log="Day 305. One of these calling another. The room that gets opened has its own everything: its own paces, its own answer. When it hands the answer back it shuts and takes its names with it. The outer room never sees them and does not need to.",
    goal_output=["5", "4"], unlocks="light.report=10", panel="REPORT", run_cost=18, predicted_run_cost=6,
))

S4 = ensure_script("DA_S4_LadderDown", "COUNTDOWN  the ladder down", """# countdown
# ilse, day 318. write the bottom step first or it never stops. i did not.

def countdown(n):
    if n == 0:
        return "BOTTOM"
    print(n)
    return countdown(n - 1)

print(countdown(3))
""", ["print"], dict(
    level_id="a4-l04", act=ACT, tier=4, editable_lines=[10],
    concepts=["recursion", "call-stack", "return-value", "conditional"],
    predictions=[
        prediction("p1", "How many numbers appear before the last line of the report?", 10, 1, "count", [
            option("a", "3"),
            option("b", "4", "fencepost", "The room with nought in it returns before it prints. Three, two, one."),
            option("c", "1", "loop-runs-once", "Each room calls the next one down until the bottom step. Three rooms print."),
        ], "a"),
        prediction("p2", "The innermost room reaches line 6. What does line 10 end up showing?", 6, 1, "output", [
            option("a", "BOTTOM"),
            option("b", "None", "recursion-no-return", "Line 8 hands back whatever the room below handed it. BOTTOM climbs every room to the top."),
            option("c", "0", "fencepost", "Nought is the question at the bottom step, not the answer. The answer is the word."),
        ], "a"),
    ],
    hints=[hint(1, "Line 5 is the bottom step. Everything above it only happens on the way down; the answer is made at the bottom and travels back up."),
           hint(2, "Line 8 does two things in one: it calls the next room down, waits for it, and then hands whatever came back straight out again without changing it.", 6)],
    intro="A piece of work that calls itself. Each call opens a room inside the last one, and they shut in the reverse order they opened, which means the answer has to come back up through every one of them. Rewind and watch the recorder. This is the script the recorder was built for.",
    outro="Three, two, one, and BOTTOM comes back up through four rooms to reach the print that asked for it. The core ring lights.",
    log="Day 318. It calls itself. Write the bottom step first or it never stops, and I did not, and it did not, and the power went at four in the morning. The part I keep having to hold on to is that the answer has to come back up. Every room is still open and still waiting while the one inside it works.",
    goal_output=["3", "2", "1", "BOTTOM"], unlocks="light.core=10", panel="COUNTDOWN", run_cost=18, predicted_run_cost=6,
))

S5 = ensure_script("DA_S4_HowLong", "HOW LONG", """# how long
# ilse, day 334. 224 metres in twelve months. i have checked it four times.
# the mast sits 216 metres from the edge. i could not do the arithmetic twice.

def per_month(total, months):
    return total // months

def months_to(distance, rate):
    if distance <= 0:
        return 0
    return 1 + months_to(distance - rate, rate)

rate = per_month(224, 12)

print(rate)
print(months_to(216, rate))
""", ["print"], dict(
    level_id="a4-l05", act=ACT, tier=4, editable_lines=[13],
    concepts=["recursion", "function-def", "parameters", "return-value", "call-stack", "arithmetic"],
    predictions=[
        prediction("p1", "What rate does line 15 report, in metres a month?", 15, 1, "value", [
            option("a", "18"),
            option("b", "18.67, because 224 does not divide evenly by 12", "type-confusion", "Two slashes: the remainder is thrown away and the answer is a whole number. Eighteen."),
            option("c", "12", "arg-param-identity", "Twelve is the months handed in. The rate is what comes back."),
        ], "a"),
        prediction("p2", "And line 16. How many months does it come back with?", 16, 1, "value", [
            option("a", "12"),
            option("b", "None", "recursion-no-return", "Every room hands one plus the room below back up. Nothing is lost on the way."),
            option("c", "13", "fencepost", "Twelve rooms take eighteen off two hundred and sixteen; the thirteenth finds nothing left and hands back nought."),
            option("d", "216", "arg-param-identity", "Two hundred and sixteen is the distance handed in, not the months that come back."),
        ], "a"),
    ],
    hints=[hint(1, "Two slashes, as at the mixer and in the two rooms: the remainder is thrown away and the answer is a whole number."),
           hint(2, "months_to takes one month off the distance each time it calls itself, and adds one to whatever comes back. The rooms open all the way down to nothing left, and then the ones come back up.", 6),
           hint(3, "Rewind through it and watch the recorder. There is a room for every month, and counting the rooms is the same as reading the answer.", 12)],
    intro="This is the last script Ilse wrote. It is dated day three hundred and thirty-four, it is nine lines long, and it uses everything you have learned to read. She did not run it. The comment says why. Run it yourself, and then check the date on the door log against today.",
    outro="Eighteen metres a month. Twelve months to the edge, and she wrote that eleven months ago.",
    log="Day 334. I have checked it four times and it does not get better. Two hundred and twenty-four metres in twelve months and the mast sits two hundred and sixteen from the edge. Do the arithmetic yourself; I could not make myself do it twice. The long-range set has been dead since the spring and nobody has answered the beacon since before that, so there is no version of this where I sit here with it. Skarvet is forty kilometres and the depot has a working transmitter. I am taking the survey book and the sledge. If you are reading this then you got in, which means the doors held, which means I was right about at least one thing.",
    goal_output=["18", "12"], unlocks="light.doorlog=10", panel="HOW LONG", run_cost=18, predicted_run_cost=6,
))

S6 = ensure_script("DA_S4_Transmission", "MAST  the transmission", """# the mast is up.
# it takes one line per figure: the label, one space, then the number.
# it calls report_line for each of the three, in the order below.
# write report_line. the mast will not wait and i am not here to do it.

print(report_line("DRIFT", 224))
print(report_line("RATE", 18))
print(report_line("MONTHS", 12))
""", ["print", "str"], dict(
    level_id="a4-l06", act=ACT, tier=4, free_edit=True,
    concepts=["function-def", "parameters", "return-value", "type-coercion", "string-ops"],
    tests=[test_case("the mast is handed three lines, each one a label and a figure with a single space between them", output=["DRIFT 224", "RATE 18", "MONTHS 12"])],
    hints=[hint(1, "Run it once before you write anything. The station will tell you exactly what it cannot find, and the name it gives you is the name your piece of work needs."),
           hint(2, "Two things are handed over each time, so two names go in the brackets. What comes back is one piece of text, which means the number has to be told to become text first, as it was on the manifest.", 6),
           hint(3, "The three calls below expect the work to already exist by the time they run, so it has to be written above them. Leave those three lines alone; they are the mast's, not yours.", 12)],
    intro="The beacon you rebuilt on your first week is the only transmitter left standing, and it will take a report if something hands it one line at a time. The three calls are already written and the mast is already listening. What is missing is the piece of work they are calling, and there is nobody else here to write it.",
    outro="DRIFT 224. RATE 18. MONTHS 12. The mast holds the carrier for four seconds after the last line, the way it always did, and then the corridor is quiet. Somebody at Skarvet will read it in the morning. Whether anybody read the last one is not a thing this station can tell you.",
    log="There is no entry after 334 in her book. It stops there, and the sledge is gone from the rack by the west door, and the survey book is gone with it. This page is the one you are writing, and it is the first line in the log that is not in her handwriting.",
    panel="MAST", unlocks="light.mast=10", run_cost=12, predicted_run_cost=6,
))

# --- the level -----------------------------------------------------------------------------
# The Reactor: the approach you arrive in, the control corridor, the core hall north, the
# depot-form room south, then the corner and the leg to the west door, with the mast and the
# empty sledge rack. The west door opens on the sector: the way out, the way she went.
begin_level(LEVEL)

place_mesh("SM_Kit_CorridorSegment", (-400, 0, 0), label="Approach")
sealed_door((-415, 0, 0), 0, "Sector3_Gate")
place_mesh("SM_Kit_CorridorSegment", (0, 0, 0), label="Corridor_A_Control")
place_mesh("SM_Kit_CorridorDoorway", (400, 0, 0), label="Corridor_B_DoorwayNorth")
place_mesh("SM_Kit_CorridorSegment", (800, 0, 0), label="Corridor_C")
place_mesh("SM_Kit_CorridorDoorway", (1200, 0, 0), rot=(0, 180, 0), label="Corridor_D_DoorwaySouth")
place_mesh("SM_Kit_CorridorCorner", (1600, 0, 0), label="Corner")
place_mesh("SM_Kit_CorridorSegment", (1750, 150, 0), rot=(0, 90, 0), label="Corridor_E_Leg")
place_mesh("SM_Kit_CorridorSegment", (1750, 550, 0), rot=(0, 90, 0), label="Corridor_F_WestDoor")
room_off(600, 0, +1, "Room_Core")
room_off(1400, 0, -1, "Room_DepotForm")

gits_door((-15, 0, 0), 0, "control", "Door_Control")
# the west door: the way out. It opens on the sector and leads nowhere the station can take you.
west = gits_door((1750, 995, 0), 90, "west", "Door_West")

# 1. the conversion and 2. the rooms, in the depot-form room
terminal((1270, -300, 0), 180, S1, "T1_Conversion")
panel((1251, -440, 150), 180, S1, "P1_DepotForm")
lamp((1251, -440, 240), "depot", "Lamp_Depot", full=5.0, color=(0.95, 0.9, 0.75, 1.0))
terminal((1530, -400, 0), 0, S2, "T2_Rooms")
panel((1549, -280, 150), 0, S2, "P2_Tally")
lamp((1549, -280, 240), "tally", "Lamp_Tally", full=5.0, color=(0.95, 0.9, 0.75, 1.0))
# 3. two rooms, corridor A south wall
terminal((200, -128, 0), -90, S3, "T3_TwoRooms")
panel((80, -149, 150), -90, S3, "P3_Report")
lamp((80, -120, 240), "report", "Lamp_Report", full=5.0, color=(0.95, 0.9, 0.75, 1.0))
# 4. the ladder down, in the core hall, with the core and its coolant pipes
terminal((470, 400, 0), 180, S4, "T4_LadderDown")
panel((451, 300, 150), 180, S4, "P4_Countdown")
place_mesh("SM_Kit_ReactorCore", (600, 470, 0), label="ReactorCore")
place_mesh("SM_Kit_CoolantPipe", (600, 300, 250), rot=(0, 90, 0), label="Coolant_1")
place_mesh("SM_Kit_CoolantPipe", (740, 400, 250), rot=(0, 0, 0), label="Coolant_2")
lamp((600, 470, 140), "core", "Lamp_Core", full=10.0, color=(0.5, 0.9, 1.0, 1.0))
# 5. how long, corridor C north wall; the door log lamp is over the entry
terminal((900, 128, 0), 90, S5, "T5_HowLong")
panel((1030, 149, 150), 90, S5, "P5_HowLong")
lamp((-200, 0, 280), "doorlog", "Lamp_DoorLog", full=6.0, color=(1.0, 0.62, 0.15, 1.0))
# 6. the transmission, on the leg by the west door, with the mast and the empty rack
terminal((1880, 250, 0), 0, S6, "T6_Transmission")
panel((1894, 400, 150), 0, S6, "P6_Mast")
place_mesh("SM_Kit_Mast", (1640, 800, 0), label="Mast")
lamp((1640, 800, 290), "mast", "Lamp_Mast", full=8.0, color=(1.0, 0.35, 0.2, 1.0), beacon=True)
place_mesh("SM_Kit_SledgeRack", (1880, 850, 0), rot=(0, 180, 0), label="Rack_Sledge_Empty")

ceiling_light((-200, 0, 299), "approach", 3.0, "Light_Approach")
for i, loc in enumerate(((600, 0, 299), (1400, 0, 299), (1750, 150, 299), (1750, 550, 299))):
    ceiling_light(loc, "corridor", 2.5, f"Light_Corridor_{i+1}")
ceiling_light((600, 250, 299), "core", 2.0, "Light_CoreHall")
ceiling_light((1400, -380, 299), "depotform", 3.0, "Light_DepotFormRoom")
emergency_lights([(400, 0, 285), (1200, 0, 285), (1750, 300, 285), (600, 400, 285), (1400, -400, 285)])
fill_light((600, 470, 200), 1.5, 600.0, "Fill_Core", color=(0.45, 0.8, 1.0, 1.0))
fill_light((-340, -110, 40), 0.8, 400.0, "Fill_Approach")
fill_light((1750, 900, 140), 1.0, 500.0, "Fill_WestDoor", color=(0.7, 0.8, 1.0, 1.0))

wall_gauge((150, -149, 170), -90, "PowerGauge")
generator((1100, -120, 0), -90)

station({"door.control": False, "door.west": False},
        {"light.depot": 0.0, "light.tally": 0.0, "light.report": 0.0, "light.core": 0.0, "light.doorlog": 0.0, "light.mast": 0.0},
        90, 45,
        "That is the last of them. Every script she left runs, and the one she did not write is written. The west door is released. She went that way with the sledge and the survey book, eleven months ago, forty kilometres to Skarvet. The report is on the mast. Go and see if the doors held.",
        "door.west=true")
exposure_and_start((-250, 0, 100))
save_level(LEVEL)
