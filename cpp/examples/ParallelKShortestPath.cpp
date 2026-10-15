#include <hipop/graph.h>
#include <hipop/graph_path.h>
#include <hipop/graph_path_cost.h>
#include <hipop/shortest_path.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>


int main()
{
    hipop::OrientedGraph G;

    double inf = std::numeric_limits<double>::infinity();

    G.AddNode("0", 0, 0);
    G.AddNode("1", 100, 0);
    G.AddNode("2", 200, 0);
    G.AddNode("3", 300, 0);

    G.AddLink("0_1", "0", "1", 100, {{"PersonalVehicle", {{"travel_time", 10}}}}, "CAR");
    G.AddLink("1_2", "1", "2", 100, {{"PersonalVehicle", {{"travel_time", 20}}}}, "CAR");
    G.AddLink("2_3", "2", "3", 100, {{"PersonalVehicle", {{"travel_time", 10}}}}, "CAR");


    std::vector<std::string> origins = {"0"};
    std::vector<std::string> destinations = {"3"};
    //std::vector<int> kPtahs = {4, 4, 4, 4}
    std::vector<std::unordered_map<std::string, std:: string> > vecMapLabelCosts = {
        {{"CAR", "PersonalVehicle"}},

    };

    G.ShowNodes();
    G.ShowLinks();

    //std::vector<std::string> links_pb = G.GetLinksWithoutCost("time", vecMapLabelCosts[0]);
    //std::cout<< links_pb.size() << std::endl;

    //auto paths = hipop::parallelKShortestPath(G, origins, destinations, "time", vecMapLabelCosts, {}, 0.1, 0.95, 10, 10, kPaths, 4);
    auto paths = hipop::parallelDijkstra(G, origins, destinations, vecMapLabelCosts, "time", 1, {});
    const std::optional<hipop::PathCost> &path = paths[0];

    if (path.has_value()) {
        std::cout << "Cost: " << path->second << std::endl;
        std::cout << "Path size (i.e. number of links): " << path->first.size() << std::endl;
        std::cout << "Path: " << path->first << std::endl;
    }

    // Update costs
    std::unordered_map<std::string, mapcosts> maplinkcosts;
    maplinkcosts= {{"1_2",{{"PersonalVehicle", {{"time", inf}}}}}};
    G.UpdateCosts( maplinkcosts );

    auto paths2 = hipop::parallelDijkstra(G, origins, destinations, vecMapLabelCosts, "time", 1, {});
    const std::optional<hipop::PathCost> &path2 = paths2[0];

    if (path2.has_value()) {
        std::cout << "Cost: " << path2->second << std::endl;
        std::cout << "Path size (i.e. number of links): " << path2->first.size() << std::endl;
        std::cout << "Path: " << path2->first << std::endl;
    }

    return EXIT_SUCCESS;
}
