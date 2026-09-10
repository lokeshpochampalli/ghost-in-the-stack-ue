door_open = True
temp = 6

if door_open and temp > 5:
    print("door open and warming")

if not door_open or temp > 5:
    print("one of the two")

print(False and 1 / 0)
