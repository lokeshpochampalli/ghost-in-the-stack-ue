# ilse: the cold store alarm. four is fine, six is not.
temp = 6

if temp < 4:
    print("too cold")
elif temp <= 5:
    print("holding")
else:
    print("warm, check the door")
