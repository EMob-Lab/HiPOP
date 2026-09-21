import matplotlib.pyplot as plt

from hipop.graph import generate_manhattan
from hipop.render import render_oriented_graph

G = generate_manhattan(3, 10)

fig, ax = plt.subplots()
render_oriented_graph(ax, G)
plt.show()
