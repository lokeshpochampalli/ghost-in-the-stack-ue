# ilse: the pulse ladder. it calls itself, which took me a week to believe.
def ladder(n):
    if n <= 1:
        return 1
    return n * ladder(n - 1)

print(ladder(4))
