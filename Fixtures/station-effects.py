# ilse: valve, then heat, then let it settle
open_valve("main")
set_heater(3)
wait(2)
log("cycle started")
close_valve("main")
