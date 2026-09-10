def first_over(limit):
    for reading in [2, 4, 6, 8]:
        if reading > limit:
            return reading
    return -1

print(first_over(5))
