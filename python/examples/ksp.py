from importlib.resources import path
from hipop.graph import OrientedGraph, link_to_dict, graph_to_dict
from hipop.shortest_path import parallel_k_shortest_path, compute_path_length 
from pprint import pprint

G = OrientedGraph()

G.add_node("0", 0, 0.,'pv_layer')
G.add_node("1", 1, 0.,'pv_layer')
G.add_node("2", 1, 1.,'pv_layer')
G.add_node("3", 0, 1.,'pv_layer')

G.add_link("0_1", "0", "1", 1, {'PV':{'travel_time':15}},'pv_layer')
G.add_link("1_2", "1", "2", 1,{'PV':{'travel_time':12}},'pv_layer')
G.add_link("2_3", "2", "3", 1.44,{'PV':{'travel_time':100}},'pv_layer')

print(graph_to_dict(G))


N = int(1e6)
N=1
origins = ["0" for _ in range(N)]
destinations = ["3" for _ in range(N)]

# Usable layer
layers = [{'pv_layer'} for _ in range(N)]

# Usable services (for each layer)
s=dict()
s['pv_layer']='PV'
services = [s for _ in range(N)]

# Number of k shortest paths to calculate
kpath=[1 for _ in range(N)]

# Number of threads to use
nthread=8

# The maximal difference between the cost of the first computed shortest path and the cost of the next ones
min_dist=2

# The maximal distance in common between the first shortest path found and the next ones
max_dist=0.8

links = G.get_links_without_cost('travel_time',{'pv_layer':'PV','metro':'M'})

if len(links) == 0:
    paths = parallel_k_shortest_path(G, origins, destinations, 'travel_time',services, layers,min_dist,max_dist,1.5, 1000, kpath,nthread)
    pprint(paths)

    #print('Lengths: ',compute_path_length(G,paths[0][0][0]),compute_path_length(G,paths[0][1][0]))

                                                     