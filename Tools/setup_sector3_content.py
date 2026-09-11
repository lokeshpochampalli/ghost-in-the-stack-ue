# Runs inside Unreal's Python (Tools/ue_remote_python.py Tools/setup_sector3_content.py).
# Sector 3, Logistics: eight systems re-authored from the reference's Act 3 (a3-l01 to a3-l08),
# loops, lists and indexing, and the level they live in. Safe to re-run.
import sys, os
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__))))
from gits_content_lib import *

ACT = 3
LEVEL = "/Game/Sectors/L_Sector3_Logistics"
NEXT = "/Game/Sectors/L_Sector4_Reactor"

S1 = ensure_script("DA_S3_PurgeCycle", "INTAKE  purge cycle", """# intake purge
# ilse, day 195. it keeps going while the count is still above zero.

remaining = 3

while remaining > 0:
    print(remaining)
    remaining = remaining - 1

print("CLEAR")
""", ["print"], dict(
    level_id="a3-l01", act=ACT, tier=3, editable_lines=[4],
    concepts=["while-loop", "comparison", "output", "reassignment"],
    predictions=[
        prediction("p1", "How many numbers does the purge print before CLEAR?", 10, 1, "count", [
            option("a", "3"),
            option("b", "1", "loop-runs-once", "The loop goes round while the count is above zero. Three, two, one: three passes, then CLEAR."),
            option("c", "4", "fencepost", "Zero is not above zero. The question on line 6 says no before a fourth pass."),
        ], "a"),
        prediction("p2", "The second time line 7 runs, what number does it show?", 7, 2, "output", [
            option("a", "2"),
            option("b", "3", "loop-runs-once", "Line 8 took one off at the end of the first pass. The second pass shows two."),
            option("c", "1", "fencepost", "One is the third pass. The second shows two."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'it keeps going while the count is still above zero.' Follow the count rather than the question: it starts at three and the loop stops when it is no longer above zero."),
           hint(2, "Line 6 is asked before every pass, including the first. When it is finally false the loop is done and line 10 runs once.", 5)],
    intro="Spring, and the intake is silted. The purge runs a set number of passes and counts itself down. You can change the number of passes on line 4. Read what it does with it first, and note which line is the one that makes it stop.",
    outro="Three passes and CLEAR. Water moves through the intake for the first time since the freeze.",
    log="Day 195. The purge repeats. That is the whole idea and it took me a week to stop being frightened of it: the same three lines run again and again and the only thing that changes is the count. If the count did not come down it would run until the power went, so the line that changes it is the important one, not the line that asks.",
    goal_output=["3", "2", "1", "CLEAR"], unlocks="vent.intake=true", panel="INTAKE PURGE", run_cost=14, predicted_run_cost=5,
))

S2 = ensure_script("DA_S3_SensorSweep", "CHANNEL BOARD  sweep", """# sensor sweep
# ilse, day 208. range stops before the number you hand it.

total = 0

for reading in range(4):
    total = total + reading

print(total)
""", ["print", "range"], dict(
    level_id="a3-l02", act=ACT, tier=3, editable_lines=[4],
    concepts=["for-loop", "range", "accumulator"],
    predictions=[prediction("p1", "What does line 9 report as the total?", 9, 1, "value", [
        option("a", "6"),
        option("b", "10", "fencepost", "range(4) is nought, one, two, three. It stops before four. Six."),
        option("c", "3", "accumulator-reset", "total is made once, on line 4, outside the loop. Each pass adds to what it already holds."),
        option("d", "The station stops. total cannot be equal to total plus a number.", "assignment-as-equality", "Line 7 is not a claim. It is an instruction: read the old total, add, put the answer back under the same name."),
    ], "a")],
    hints=[hint(1, "Ilse's reminder is on line 2. Write out the four values the sweep walks before you add anything up."),
           hint(2, "total is made once, on line 4, and the loop adds to whatever it is already holding. Line 7 is an instruction, not a claim: read the old total, add, put the answer back.", 5)],
    intro="Four sensor channels, numbered from nought, and a total that grows as the sweep walks them. Ilse wrote her own reminder about where range stops into the top of the file, which tells you which mistake she kept making.",
    outro="Six. The sweep closes and the channel board goes quiet.",
    log="Day 208. range stops before the number you hand it. Four means nought, one, two, three, and I have written that at the top of the file because I will forget it again. The total is not reset inside the loop, which is the other thing I got wrong: it has to be made once, outside, or every pass wipes the one before.",
    goal_output=["6"], unlocks="light.channelboard=0", panel="CHANNEL BOARD", run_cost=14, predicted_run_cost=5,
))

S3 = ensure_script("DA_S3_BallastTally", "BALLAST  the tally", """# ballast tally
# ilse, day 219. += is the same instruction written shorter.

tanks = 0
litres = 0

for tank in range(1, 4):
    tanks += 1
    litres += tank * 100

print(tanks)
print(litres)
""", ["print", "range"], dict(
    level_id="a3-l03", act=ACT, tier=3, editable_lines=[4, 5],
    concepts=["augmented-assignment", "accumulator", "range", "for-loop", "arithmetic"],
    predictions=[
        prediction("p1", "How many tanks does line 11 count?", 11, 1, "value", [
            option("a", "3"),
            option("b", "4", "fencepost", "range(1, 4) starts at one and stops before four. One, two, three."),
            option("c", "1", "loop-runs-once", "The loop goes round once for each tank. Three tanks, three passes."),
        ], "a"),
        prediction("p2", "And how many litres does line 12 report?", 12, 1, "value", [
            option("a", "600"),
            option("b", "300", "accumulator-reset", "litres keeps growing. A hundred, then three hundred, then six hundred."),
            option("c", "1000", "fencepost", "There is no tank four. One, two, three hundred: six hundred."),
            option("d", "100", "loop-runs-once", "Three passes, each adding its tank times a hundred."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: '+= is the same instruction written shorter.' Read line 9 as: take what litres is holding, add this tank, put the answer back."),
           hint(2, "range with two numbers starts at the first and stops before the second, so the tank numbers are one, two and three. Each one contributes a hundred times itself.", 5)],
    intro="Three ballast tanks, filled in order, and two totals kept at once. This is the first script where Ilse uses the short form of adding to a name. She says it means the same thing, and she is right, but you have to be able to read it before you can believe her.",
    outro="Three tanks, six hundred litres. The trim gauge settles level and the ballast line starts to move.",
    log="Day 219. Started writing += instead of writing the name out twice. It is the same instruction. The name is read, the addition happens, the answer goes back under the same name, exactly what I was writing before, with less of it. I resisted this for months because it looked like it was doing something clever and it is not.",
    goal_output=["3", "600"], unlocks="conveyor.ballast=true", panel="BALLAST TALLY", run_cost=14, predicted_run_cost=5,
))

S4 = ensure_script("DA_S3_MarkerLine", "SURVEY  the markers", """# shelf markers
# ilse, day 231. metres from each marker to the mast, in the order i walk them.

markers = [412, 407, 401, 396]

print(markers[2])
print(len(markers))
print(markers[-1])
""", ["print", "len"], dict(
    level_id="a3-l04", act=ACT, tier=3, editable_lines=[4],
    concepts=["list-literal", "indexing"],
    predictions=[
        prediction("p1", "Line 6 asks for position 2. Which marker is that?", 6, 1, "value", [
            option("a", "401"),
            option("b", "407", "index-from-one", "Positions count from nought. Nought is 412, one is 407, two is 401."),
            option("c", "The station stops. 2 is not one of the markers.", "type-confusion", "The number in the brackets is a position along the list, not a marker to look for."),
        ], "a"),
        prediction("p2", "What does line 7 measure the list as?", 7, 1, "value", [
            option("a", "4"),
            option("b", "3", "fencepost", "len counts the markers themselves: four. The highest position is one less than that."),
            option("c", "412", "type-confusion", "len is how many, not what is first."),
        ], "a"),
        prediction("p3", "Line 8 asks for position minus one. What comes back?", 8, 1, "value", [
            option("a", "396"),
            option("b", "412", "operator-confusion", "Minus one counts backwards from the end. The last marker, not the first."),
            option("c", "The station stops. There is no marker at minus one.", "type-confusion", "A negative position is allowed. It counts from the end."),
        ], "a"),
    ],
    hints=[hint(1, "Her log entry for this one says it: the positions count from nought. So the first marker is at position 0, not position 1."),
           hint(2, "len counts the markers themselves, which is one more than the highest position. A negative position counts backwards from the end, so minus one is the last one.", 5)],
    intro="Four survey markers out on the shelf, and their distances from the mast held in one name instead of four. This is the first of Ilse's survey scripts. There are more of them than there are of anything else she wrote.",
    outro="Four markers, and the nearest of them three hundred and ninety-six metres out. The marker shelf lights, one lamp for each.",
    log="Day 231. Put the marker distances in a list. One name, four numbers, in the order I walk them. The positions count from nought, which is the second thing about this language that I had to write down and look at for a while.",
    goal_output=["401", "4", "396"], unlocks="light.markers=10", panel="MARKERS", run_cost=16, predicted_run_cost=5,
))

S5 = ensure_script("DA_S3_TheWalk", "SURVEY  the walk", """# marker drift, month on month
# ilse, day 244. how far each marker moved since the last walk. metres.

moves = [3, 5, 4, 9]
drift = 0

for move in moves:
    drift += move

print(drift)
print(moves[len(moves) - 1])
""", ["print", "len"], dict(
    level_id="a3-l05", act=ACT, tier=3, editable_lines=[4, 5],
    concepts=["for-loop", "accumulator", "indexing", "augmented-assignment"],
    predictions=[
        prediction("p1", "What total does line 10 report?", 10, 1, "value", [
            option("a", "21"),
            option("b", "9", "accumulator-reset", "drift is made once, before the loop, and kept between passes. Three, eight, twelve, twenty-one."),
            option("c", "3", "loop-runs-once", "The loop hands over every number in the list, one pass each. Four passes."),
        ], "a"),
        prediction("p2", "Line 11 asks for the position one below the length. What comes back?", 11, 1, "value", [
            option("a", "9"),
            option("b", "4", "type-confusion", "len is four; one below that is position three; position three holds nine. The number in the brackets is a position, not a value."),
            option("c", "The station stops. There is no position four.", "fencepost", "Four minus one is three, and position three is the last one. It is there."),
        ], "a"),
    ],
    hints=[hint(1, "The loop hands you the numbers themselves, not their positions. drift is made once, before the loop, and kept between passes."),
           hint(2, "There are four moves, so len is four, and one below that is position three. Positions run 0, 1, 2, 3, so position three is the last one.", 5)],
    intro="Same four markers, a month later, and this time the list holds how far each one has moved rather than where it is. The loop walks the list itself instead of counting positions. What it adds up is the reason she kept walking out there.",
    outro="Twenty-one metres in a month, across four markers. The number goes in the survey book and the survey book goes back on the shelf.",
    log="Day 244. Walked the markers again. You can hand a loop the list itself and it gives you the numbers one at a time: no positions, no counting, no chance of asking for one that is not there. I should have found that out in March.",
    goal_output=["21", "9"], unlocks="light.survey=10", panel="SURVEY  drift", run_cost=14, predicted_run_cost=5,
))

S6 = ensure_script("DA_S3_TheGrid", "SHELF GRID  the check", """# shelf grid check
# ilse, day 256. the inner walk finishes before the outer one moves.

cells = []
row = 0

while row < 3:
    for column in range(2):
        cells.append(row * 10 + column)
    row += 1

print(len(cells))
print(cells[0])
print(cells[3])
""", ["print", "range", "len"], dict(
    level_id="a3-l06", act=ACT, tier=3, editable_lines=[7, 8, 9, 10],
    concepts=["nested-loop", "while-loop", "for-loop", "list-mutation", "range"],
    predictions=[
        prediction("p1", "How many cells does line 12 count?", 12, 1, "count", [
            option("a", "6"),
            option("b", "5", "fencepost", "Three rows, two columns each. The inner walk runs to the end every time: six."),
            option("c", "3", "loop-runs-once", "The inner loop runs twice for every row. Three rows, six cells."),
        ], "a"),
        prediction("p2", "What is the first cell the grid put in the list?", 13, 1, "value", [
            option("a", "0"),
            option("b", "1", "index-from-one", "Row nought, column nought: nought times ten plus nought. Positions and rows both start at nought."),
            option("c", "10", "sequence-ignored", "row is still nought when the first cell goes in. Line 10 moves it afterwards."),
        ], "a"),
        prediction("p3", "And what is sitting at position 3?", 14, 1, "value", [
            option("a", "11"),
            option("b", "10", "index-from-one", "Positions nought to five hold 0, 1, 10, 11, 20, 21. Position three is 11."),
            option("c", "21", "fencepost", "21 is the last cell, position five. Position three is 11."),
        ], "a"),
    ],
    hints=[hint(1, "Ilse: 'the inner walk finishes before the outer one moves.' row stays at nought for both of the first two cells, and line 10 is the only thing that changes it."),
           hint(2, "Rewind it and watch the recorder: the six cells arrive in the order the list gets them, two for every row.", 5)],
    intro="Six cells to check, laid out three rows by two columns, and one loop inside another to reach them all. The outer one is a while and the inner one is a for, which is Ilse using whichever fitted rather than whichever she had just learned. The list starts empty and fills as the walk goes.",
    outro="Six cells, in the order the grid is walked. The shelf grid lights up, row by row.",
    log="Day 256. Two loops, one inside the other. The inner one runs all the way through before the outer one moves on by a single step, which means the cells come out in rows and not in columns. I drew it on paper four times before I trusted it. The outer one counts itself because I wanted to watch the counting.",
    goal_output=["6", "0", "11"], unlocks="light.grid=10", panel="SHELF GRID", run_cost=16, predicted_run_cost=5,
))

S7 = ensure_script("DA_S3_LastWalk", "SURVEY  twelve months", """# shelf survey, twelve months
# ilse, day 268
# the drift each month since i started. it is not slowing down.

monthly = [3, 5, 4, 9, 11, 14, 18, 21, 26, 31, 38, 44]
running = []
total = 0

for step in monthly:
    total += step
    running.append(total)

print(total)
print(running[5])
print(len(running))
""", ["print", "len"], dict(
    level_id="a3-l07", act=ACT, tier=3, editable_lines=[5, 6, 7],
    concepts=["for-loop", "accumulator", "list-mutation", "augmented-assignment", "indexing"],
    predictions=[
        prediction("p1", "What total does line 13 report for the twelve months?", 13, 1, "value", [
            option("a", "224"),
            option("b", "44", "accumulator-reset", "total keeps everything it has taken in. Forty-four is only the last month."),
            option("c", "12", "type-confusion", "Twelve is how many months. The total is what they add up to."),
            option("d", "3", "loop-runs-once", "Twelve passes, one for each month."),
        ], "a"),
        prediction("p2", "Line 14 asks the running list for position 5. What is kept there?", 14, 1, "value", [
            option("a", "46"),
            option("b", "14", "accumulator-reset", "running takes whatever total holds at that moment, not the month's own figure. Six months in: forty-six."),
            option("c", "224, because every entry follows total and total ends at 224", "parallel-assignment", "Each entry is a copy of total at the moment it was appended. It does not move afterwards."),
        ], "a"),
        prediction("p3", "How long is the running list by the end?", 15, 1, "count", [
            option("a", "12"),
            option("b", "11", "fencepost", "One entry per month, twelve months."),
            option("c", "1", "loop-runs-once", "The loop goes round twelve times and appends every time."),
        ], "a"),
    ],
    hints=[hint(1, "Two things change on every pass. total keeps growing, and running gets one more entry, and what it gets is whatever total is holding at that moment, not the month's own figure."),
           hint(2, "Rewind and watch the two of them together. By the sixth pass total has taken in the first six months and running has six entries, the last of which is that same number.", 6)],
    intro="Twelve months of the survey in one list, and a second list that fills as the first is read. Ilse ran this on her two hundred and sixty-eighth day and then stopped adding to the survey book. Read the numbers on line 5 before you read the code. The shape of them is the point.",
    outro="Two hundred and twenty-four metres in twelve months, and the last month alone was forty-four of them. The survey board goes red.",
    log="Day 268. Twelve months of the shelf, month by month, and a running total beside it so I can see the shape instead of the last figure. It is not slowing down. It is doing the other thing. I have written the total out three times now hoping to have made an arithmetic error and I have not made an arithmetic error.",
    goal_output=["224", "46", "12"], unlocks="light.board=10", panel="SURVEY BOARD", run_cost=16, predicted_run_cost=6,
))

S8 = ensure_script("DA_S3_DepotTally", "DEPOT  the tally", """# ballast tally for the depot
# ilse, day 275. four lines, in this order: how many tanks, the total litres,
# the largest single tank, and how many pairs of tanks read the same.

tanks = [180, 240, 95, 240, 60]
""", ["print", "len", "range"], dict(
    level_id="a3-l08", act=ACT, tier=3, free_edit=True,
    concepts=["for-loop", "nested-loop", "accumulator", "list-literal", "indexing"],
    tests=[test_case("the depot gets four lines: how many tanks, the total litres, the largest single tank, then how many pairs of tanks read the same", output=["5", "815", "240", "1"])],
    hints=[hint(1, "The first three you have written already this act: a count, a total, and a name that only changes when the loop meets something bigger."),
           hint(2, "A pair is two positions, so the last figure needs one walk inside another, and the inner walk starts one past the outer one, or every pair gets counted twice and every tank gets paired with itself.", 6),
           hint(3, "For the largest, start it at the first reading rather than at nought. Four prints, in the order the depot asks for them, and nothing else on the report.", 12)],
    intro="The depot form wants four figures off the ballast gauges and Ilse never automated it. She did it by hand every month, in pencil, in the margin. The readings are on line 5 and they are as the gauge gave them. Four lines out, in the order the form asks for them, and the last of them is the one she hated most.",
    outro="Five tanks, eight hundred and fifteen litres, the biggest of them two hundred and forty, and one pair reading the same. The form goes on the outbound line, where four of her forms are already waiting.",
    log="Day 275. Did the depot form by hand again. There is no reason for that any more: I can count a list and total a list and find the largest thing in a list, and I have written all three of those this month for other jobs. I think I keep doing it in pencil because it is fifteen minutes where I am not thinking about the shelf.",
    panel="DEPOT FORM", unlocks="conveyor.outbound=true", run_cost=12, predicted_run_cost=6,
))

# --- the level -----------------------------------------------------------------------------
# Logistics: the drone bay you arrive in, a long loading corridor with the ballast line and the
# shelf racks, the survey room north, the depot room south with the outbound line, the corner and
# the leg to the reactor gate.
begin_level(LEVEL)

place_mesh("SM_Kit_CorridorSegment", (-400, 0, 0), label="DroneBay")
sealed_door((-415, 0, 0), 0, "Sector2_Gate")
place_mesh("SM_Kit_CorridorSegment", (0, 0, 0), label="Corridor_A")
place_mesh("SM_Kit_CorridorDoorway", (400, 0, 0), label="Corridor_B_DoorwayNorth")
place_mesh("SM_Kit_CorridorSegment", (800, 0, 0), label="Corridor_C")
place_mesh("SM_Kit_CorridorDoorway", (1200, 0, 0), rot=(0, 180, 0), label="Corridor_D_DoorwaySouth")
place_mesh("SM_Kit_CorridorCorner", (1600, 0, 0), label="Corner")
place_mesh("SM_Kit_CorridorSegment", (1750, 150, 0), rot=(0, 90, 0), label="Corridor_E_Leg")
place_mesh("SM_Kit_CorridorSegment", (1750, 550, 0), rot=(0, 90, 0), label="Corridor_F_Approach")
room_off(600, 0, +1, "Room_Survey")
room_off(1400, 0, -1, "Room_Depot")

gits_door((-15, 0, 0), 0, "bay", "Door_Bay")
sector_gate((1750, 995, 0), 90, "door.reactor", NEXT, "SECTOR 4  REACTOR", "Gate_Sector4",
            "Sealed. The reactor door takes the supply line running: every list counted, every loop walked.")

# 1. the purge cycle, in the drone bay, with the intake vent
terminal((-200, 128, 0), 90, S1, "T1_Purge")
panel((-320, 149, 150), 90, S1, "P1_Purge")
vent((-80, 149, 150), 90, "intake", "Vent_Intake")
# 2. the sensor sweep, corridor A south wall; the channel board goes quiet
terminal((200, -128, 0), -90, S2, "T2_Sweep")
panel((80, -149, 150), -90, S2, "P2_ChannelBoard")
lamp((80, -120, 240), "channelboard", "Lamp_ChannelBoard", full=6.0, color=(0.3, 1.0, 0.5, 1.0), level=6.0)
# 3. the ballast tally, corridor C; the ballast line runs
terminal((900, 128, 0), 90, S3, "T3_Ballast")
panel((1030, 149, 150), 90, S3, "P3_Ballast")
conveyor((1000, -100, 0), 0, "ballast", "Conveyor_Ballast", crates=3)
# 4. the marker line and 5. the walk, in the survey room, the marker shelf lit by lamps
terminal((470, 400, 0), 180, S4, "T4_Markers")
panel((451, 300, 150), 180, S4, "P4_Markers")
terminal((730, 300, 0), 0, S5, "T5_Walk")
panel((749, 420, 150), 0, S5, "P5_Survey")
place_mesh("SM_Kit_Shelf", (730, 520, 0), rot=(0, 0, 0), label="Shelf_Survey")
for i in range(4):
    lamp((560 + i * 40, 520, 230), "markers", f"Lamp_Marker_{i+1}", full=2.5, color=(1.0, 0.9, 0.6, 1.0))
lamp((749, 420, 240), "survey", "Lamp_Survey", full=5.0, color=(0.95, 0.9, 0.75, 1.0))
# 6. the grid, corridor C south wall; the shelf grid of six lamps
terminal((1100, -128, 0), -90, S6, "T6_Grid")
panel((960, -149, 150), -90, S6, "P6_Grid")
place_mesh("SM_Kit_Shelf", (1230, -130, 0), rot=(0, -90, 0), label="Shelf_Grid_1")
for r in range(3):
    for c in range(2):
        lamp((1200 + c * 30, -125, 70 + r * 70), "grid", f"Lamp_Grid_{r}{c}", full=2.0, color=(0.6, 0.9, 1.0, 1.0))
# 7. the last walk and 8. the depot tally, in the depot room, with the outbound line
terminal((1270, -300, 0), 180, S7, "T7_LastWalk")
panel((1251, -440, 150), 180, S7, "P7_SurveyBoard")
lamp((1251, -440, 240), "board", "Lamp_SurveyBoard", full=7.0, color=(1.0, 0.25, 0.15, 1.0))
terminal((1530, -400, 0), 0, S8, "T8_DepotTally")
panel((1549, -280, 150), 0, S8, "P8_DepotForm")
conveyor((1400, -520, 0), 0, "outbound", "Conveyor_Outbound", crates=4)

# dressing: crates, shelves along the corridor, the parked drone
for i, (x, y) in enumerate(((300, 110), (300, 110), (1100, 110), (1650, 300))):
    place_mesh("SM_Kit_Crate", (x, y, 60 * (i % 2)), rot=(0, 5 * i, 0), label=f"Crate_{i+1}")
place_mesh("SM_Kit_Shelf", (130, 130, 0), rot=(0, 90, 0), label="Shelf_Corridor_1")
place_mesh("SM_Kit_Drone", (-300, -90, 0), rot=(0, 30, 0), label="Drone_Bay")
place_mesh("SM_Kit_SledgeRack", (1880, 700, 0), rot=(0, 0, 0), label="Rack_Spare")

ceiling_light((-200, 0, 299), "bay", 3.0, "Light_Bay")
for i, loc in enumerate(((600, 0, 299), (1400, 0, 299), (1750, 150, 299), (1750, 550, 299))):
    ceiling_light(loc, "corridor", 2.5, f"Light_Corridor_{i+1}")
ceiling_light((600, 380, 299), "survey", 3.5, "Light_SurveyRoom")
ceiling_light((1400, -380, 299), "depot", 3.0, "Light_DepotRoom")
emergency_lights([(400, 0, 285), (1200, 0, 285), (1750, 300, 285), (600, 400, 285), (1400, -400, 285)])
fill_light((-340, -110, 40), 0.8, 400.0, "Fill_Bay")
fill_light((1750, 900, 140), 1.2, 500.0, "Fill_Approach")

wall_gauge((150, -149, 170), -90, "PowerGauge")
generator((600, 520, 0), 90)

station({"door.bay": False, "door.reactor": False, "vent.intake": False, "conveyor.ballast": False, "conveyor.outbound": False},
        {"light.channelboard": 6.0, "light.markers": 0.0, "light.survey": 0.0, "light.grid": 0.0, "light.board": 0.0},
        90, 45,
        "The supply line runs. Every list she kept is counted and every loop she wrote comes round the right number of times. The reactor door is released, and the last of her scripts are through there. So is the door log.",
        "door.reactor=true")
exposure_and_start((-250, 0, 100))
save_level(LEVEL)
