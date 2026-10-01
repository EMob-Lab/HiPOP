from hipop.graph import OrientedGraph, merge_oriented_graph

G = OrientedGraph()

G.add_node("0", 0, 0, 'N0')
G.add_node("1", 1, 0, 'N0')
G.add_node("2", 1, 1, 'N0')
G.add_node("3", 0, 1, 'N0')

G.add_link("0_1", "0", "1", 1, {'PV':{"time": 10}})
G.add_link("0_3", "0", "3", 1, {'PV':{"time": 11}})
G.add_link("0_2", "0", "2", 1.44, {'PV':{"time": 10}})
G.add_link("1_2", "1", "2", 4, {'PV':{"time": 10}})
G.add_link("3_2", "3", "2", 5, {'PV':{"time": 9}})
G.add_link("1_3", "1", "3", 1, {'PV':{"time": 10}})

G2 = OrientedGraph()

G2.add_node("10", 0, 0, 'N0')
G2.add_node("11", 1, 0, 'N0')
G2.add_link("10_11", "10", "11", 3, {'PV':{"time": 10}})

mergedG = merge_oriented_graph([G,G2])
print(len(mergedG.nodes))
