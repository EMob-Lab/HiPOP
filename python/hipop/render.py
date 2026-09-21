from matplotlib.axes import Axes
from matplotlib.collections import LineCollection
from matplotlib.colors import Colormap

from hipop.graph import OrientedGraph


def render_oriented_graph(
        ax: Axes,
        G: OrientedGraph,
        color: str = "black",
        linkwidth: float = 1,
        nodesize: float = 2,
        cmap: Colormap | None = None) -> None:

    x, y = zip(*[n.position for n in G.nodes.values()])
    ax.plot(x, y, 'o', markerfacecolor='white', markeredgecolor=color, fillstyle='full', markersize=nodesize)

    lines = []
    for lid, link in G.links.items():
        lines.append([G.nodes[link.upstream].position, G.nodes[link.downstream].position])
    line_segment = LineCollection(lines, linestyles='solid', linewidths=linkwidth, cmap=cmap)
    ax.add_collection(line_segment)
