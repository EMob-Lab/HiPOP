from time import time

from hipop.graph import generate_manhattan
from hipop.shortest_path import dijkstra, parallel_dijkstra

g = generate_manhattan(100, 10)

s={}
s['']='PersonalCar'

print(dijkstra(g, "NORTH_0", "EAST_0", "length",s))

N = 3000

origins = ["NORTH_0"]*N
dests = ["EAST_0"]*N

s={}
s['']='PersonalCar'
services = [s for _ in range(N)]

print("Launch")
start = time()
res = parallel_dijkstra(g, origins, dests, services, "length", 8)
end = time()
print("Done", f"[{end-start} s]")

print(res[0])
