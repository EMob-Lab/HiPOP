#pragma once

#include <hipop/graph.h>

#include <cmath>


/**
 * Simple graph with no loops, no multiple links between the same nodes, and only finite non-zero cost values:
 *
 * ```
 *         5           1.5
 *   A --------> B --------> C
 *   |\__        |           |
 *  6|   \__     |4          |4      (*) The links A -> E, D -> I and E -> I belong to the bus layer.
 *   |  7(*)\__  |           |           If the bus layer is disabled, these links are not present
 *   v         v v           v           in the graph, and therefore node I is isolated from the rest.
 *   D --------> E --------> F
 *    \   2      |     3
 *     \         |
 *      \        | 1(*)
 *       \       v
 *        -----> I
 *        4(*)
 * ```
 */
inline hipop::OrientedGraph simple_graph(bool withBusLayer) {

    hipop::OrientedGraph G;

    G.AddNode("A", 0, 0);
    G.AddNode("B", 1, 0);
    G.AddNode("C", 2, 0);
    G.AddNode("D", 0, 1);
    G.AddNode("E", 1, 1);
    G.AddNode("F", 2, 1);
    G.AddNode("I", 2, 2); // Non-reachable if there is no bus!

    G.AddLink("A_B", "A", "B", 1, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 1, {{"CAR", {{"time", 1.5}}}}, "CarLayer");
    G.AddLink("D_E", "D", "E", 1, {{"CAR", {{"time", 2}}}}, "CarLayer");
    G.AddLink("E_F", "E", "F", 1, {{"CAR", {{"time", 3}}}}, "CarLayer");
    G.AddLink("A_D", "A", "D", 1, {{"CAR", {{"time", 6}}}}, "CarLayer");
    G.AddLink("B_E", "B", "E", 1, {{"CAR", {{"time", 4}}}}, "CarLayer");
    G.AddLink("C_F", "C", "F", 1, {{"CAR", {{"time", 4}}}}, "CarLayer");

    if (withBusLayer) {
        G.AddLink("A_E", "A", "E", 3, {{"BUS", {{"time", 7}}}}, "BusLayer");
        G.AddLink("D_I", "D", "I", 2, {{"BUS", {{"time", 4}}}}, "BusLayer");
        G.AddLink("E_I", "E", "I", 1, {{"BUS", {{"time", 1}}}}, "BusLayer");
    }

    return G;
}


/**
 * Graph with 0 or +infinity cost values:
 *
 * ```
 *           1          2
 *    A --------> B -------
 *     \          |        \          (*) On this link, the cost is:
 *      \         |(*)      \             - 0 for "time"
 *       \        |          \            - +inf for "generalized_cost"
 *        \       v           v
 *         -----> C --------> D
 *          2.5         1
 * ```
 */
inline hipop::OrientedGraph graph_with_special_cost_values() {

    hipop::OrientedGraph G;

    G.AddNode("A", 0, 0);
    G.AddNode("B", 1, 0);
    G.AddNode("C", 1, 1);
    G.AddNode("D", 2, 1);

    G.AddLink("A_B", "A", "B", 1, {{"CAR", {{"time", 1}, {"generalized_cost", 1}}}}, "CarLayer");
    G.AddLink("A_C", "A", "C", 1, {{"CAR", {{"time", 2.5}, {"generalized_cost", 2.5}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 1, {{"CAR", {{"time", 0}, {"generalized_cost", INFINITY}}}}, "CarLayer");
    G.AddLink("C_D", "C", "D", 1, {{"CAR", {{"time", 1}, {"generalized_cost", 1}}}}, "CarLayer");
    G.AddLink("B_D", "B", "D", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");

    return G;
}


/**
 * Graph with a cycle. Cumulated cost on the cycle is non-zero for "time", and 0 for "generalized_cost".
 *
 * ```
 *         2            2
 *   A --------> L1 --------> B/X
 *               |^
 *               ||1 for "time"
 *               ||0 for "generalized_cost"
 *               v|
 *               L2 --------> C/Y
 *                      2
 * ```
 */
inline hipop::OrientedGraph graph_with_loop() {

    hipop::OrientedGraph G;

    G.AddNode("A", 0, 0);
    G.AddNode("B", 2, 0);
    G.AddNode("C", 2, 1);
    G.AddNode("L1", 1, 0);
    G.AddNode("L2", 1, 1);

    G.AddLink("A_L1", "A", "L1", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L1_B", "L1", "B", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L2_C", "L2", "C", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L1_L2", "L1", "L2", 1, {{"CAR", {{"time", 1}, {"generalized_cost", 0}}}}, "CarLayer");
    G.AddLink("L2_L1", "L2", "L1", 1, {{"CAR", {{"time", 1}, {"generalized_cost", 0}}}}, "CarLayer");

    // Nodes X and Y are identical to B and C respectively, with also identical links.
    // But they are added after the loop nodes, and their IDs come after the node/link IDs
    // involved in the loop. These considerations may affect the behavior of algorithms,
    // especially in case of 0 cumulated cost on the loop.
    G.AddNode("X", 2, 0);
    G.AddNode("Y", 2, 1);
    G.AddLink("L1_X", "L1", "X", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L2_Y", "L2", "Y", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");

    return G;
}


/**
 * Graph with a self-linking node (aka. a 1-length loop).
 * Cumulated cost of the self-link is non-zero for "time", and 0 for "generalized_cost".
 *
 * ```
 *         2            2
 *   A --------> L --------> B/X
 *              (*)
 *         Self-link on L, with cost:
 *         - 1 for "time"
 *         - 0 for "generalized_cost"
 * ```
 */
inline hipop::OrientedGraph graph_with_self_link() {

    hipop::OrientedGraph G;

    G.AddNode("A", 0, 0);
    G.AddNode("B", 2, 0);
    G.AddNode("L", 1, 0);

    G.AddLink("A_L", "A", "L", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L_B", "L", "B", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");
    G.AddLink("L_L", "L", "L", 1, {{"CAR", {{"time", 1}, {"generalized_cost", 0}}}}, "CarLayer");

    // Node X and link L_X are identical to B and L_B.
    // But they are added after the self-link, and their IDs come after the self-linking node ID.
    // These considerations may affect the behavior of algorithms,
    // especially in case of 0 cost on the self-link.
    G.AddNode("X", 2, 0);
    G.AddLink("L_X", "L", "X", 1, {{"CAR", {{"time", 2}, {"generalized_cost", 2}}}}, "CarLayer");

    return G;
}


/**
 * Graph with a forbidden transition:
 *
 * ```
 *          3
 *    A --------> B1
 *    |           |
 *    |2          |4        (*) On B2, the transition A -> B2 -> C is forbidden.
 *    |           |
 *    v           v
 *   B2(*) -----> C
 *           1
 * ```
 */
inline hipop::OrientedGraph graph_with_exclude_movements() {

    hipop::OrientedGraph G;

    G.AddNode("A", 0, 0);
    G.AddNode("B1", 1, 0);
    G.AddNode("B2", 0, 1, "", {{"A", {"C"}}});
    G.AddNode("C", 1, 1);

    G.AddLink("A_B1", "A", "B1", 1, {{"CAR", {{"time", 3}}}}, "CarLayer");
    G.AddLink("B1_C", "B1", "C", 1, {{"CAR", {{"time", 4}}}}, "CarLayer");
    G.AddLink("A_B2", "A", "B2", 1, {{"CAR", {{"time", 2}}}}, "CarLayer");
    G.AddLink("B2_C", "B2", "C", 1, {{"CAR", {{"time", 1}}}}, "CarLayer");

    return G;
}
