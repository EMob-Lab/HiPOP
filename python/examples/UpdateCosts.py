from importlib.resources import path
from math import inf
from hipop.graph import OrientedGraph, link_to_dict, graph_to_dict, Link
from hipop.shortest_path import parallel_k_shortest_path, compute_path_length 
from pprint import pprint

G = OrientedGraph()

G.add_node("0", 0, 0.,'pv_layer')
G.add_node("1", 1, 0.,'pv_layer')
G.add_node("2", 1, 1.,'pv_layer')
G.add_node("3", 0, 1.,'pv_layer')

G.add_link("0_1", "0", "1", 1, {'PV':{'travel_time':15}},'pv_layer')
G.add_link("1_2_bis", "1", "2", 1,{'PV':{'travel_time':30}},'pv_layer')
G.add_link("1_2", "1", "2", 1,{'PV':{'travel_time':12}},'pv_layer')


G.add_link("2_3", "2", "3", 1.44,{'PV':{'travel_time':15}},'pv_layer')

#print(type(G.nodes))
#link=G.links['0_1']
print(G.links)
print(G.nodes['1'])
print('0_1     ', G.links['0_1'])
print('1_2     ', G.links['1_2'])
print('1_2_bis ',G.links['1_2_bis'])
print('1_2     ', G.links['1_2'])

#print(G.nodes['1'].get_exits('0')[0].id,' ',G.nodes['1'].get_exits('0'))
#print(G.nodes['1'].get_exits('0')[0].id,' ',G.nodes['1'].get_exits('0'))
print(G.nodes['1'].adj['2'].id)
#print(G.links['1_2_bis'].id)
#print(graph_to_dict(G))


# N = int(1e6)
# N=1
# origins = ["0" for _ in range(N)]
# destinations = ["3" for _ in range(N)]

# # Usable layer
# layers = [{'pv_layer'} for _ in range(N)]

# # Usable services (for each layer)
# s=dict()
# s['pv_layer']='PV'
# services = [s for _ in range(N)]

# # Number of k shortest paths to calculate
# kpath=[1 for _ in range(N)]

# # Number of threads to use
# nthread=8

# # The maximal difference between the cost of the first computed shortest path and the cost of the next ones
# min_dist=2

# # The maximal distance in common between the first shortest path found and the next ones
# max_dist=0.8

# links = G.get_links_without_cost('travel_time',{'pv_layer':'PV','metro':'M'})

# if len(links) == 0:
#     paths = parallel_k_shortest_path(G, origins, destinations, 'travel_time',services, layers,min_dist,max_dist,1.5, 1000, kpath,nthread)
#     #pprint(paths)

# # Update costs
# #maplinkcosts= {"1_2":{'PV':{'travel_time':inf}}}
# #maplinkcosts= {"1_2":{'PV':{'travel_time':float("inf")}}}
# costs= {'PV':{'travel_time':float("inf")}}

# #l = G.get_link("1_2")
# #print(l)
# ##l.update_costs(costs)

# #G.update_costs(maplinkcosts)

# paths = parallel_k_shortest_path(G, origins, destinations, 'travel_time',services, layers,min_dist,max_dist,1.5, 1000, kpath,nthread)
# #pprint(paths)                                