before = read_sensor("cold_store")
wait(3)
after = read_sensor("cold_store")

print(before)
print(after)
print(after - before)
