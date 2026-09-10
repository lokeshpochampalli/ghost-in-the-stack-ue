# ilse: the depth report, factored out because i was writing it twice
def report(depth, label):
    total = depth * 2
    return label + " " + str(total)

print(report(4, "cold store"))
print(report(9, "beacon"))
