#include "hipop/graph.h"

#include <unordered_map>
#include <string>


namespace hipop
{
    /**
     * @brief Construct a simple costs map from a link length
     *
     * @param linkLength The length of a Link
     * @return mapcosts The simple constructed costs with the link length
     */
    mapcosts makeSimpleCostMap(double linkLength) {
        mapcosts costs;
        costs["PersonalCar"] = {{"length", linkLength}};
        return costs;
    }


    /**
     * @brief Construct a squared Manhattan graph
     *
     * @param n Number of nodes on each side
     * @param linkLength The length of the links
     * @return OrientedGraph*
     */
    OrientedGraph* makeManhattan(int n, double linkLength) {
        auto G = new OrientedGraph();

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                G->AddNode(std::to_string(i*n+j), i*linkLength, j*linkLength);
            }

        }

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                int ind = i*n+j;

                if(j < n-1) {
                    std::string upstream = std::to_string(ind);
                    std::string downstream = std::to_string(ind+1);
                    G->AddLink(upstream + "_" + downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
                }

                if(j > 0) {
                    std::string upstream = std::to_string(ind);
                    std::string downstream = std::to_string(ind-1);
                    G->AddLink(upstream + "_" + downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
                }

                if(i < n - 1) {
                    std::string upstream = std::to_string(ind);
                    std::string downstream = std::to_string(ind+n);

                    G->AddLink(upstream + "_" + downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
                }

                if(i > 0) {
                    std::string upstream = std::to_string(ind);
                    std::string downstream = std::to_string(ind-n);
                    G->AddLink(upstream + "_" + downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
                }
            }

        }

        int counter = 0;
        for (int i = 0; i < n; i++)
        {
            std::string upstream = "WEST_"+std::to_string(i);
            std::string downstream = std::to_string(i);
            G->AddNode(upstream, -linkLength, i*linkLength);
            G->AddLink(upstream+"_"+downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
            G->AddLink(downstream+"_"+upstream, downstream, upstream, linkLength, makeSimpleCostMap(linkLength));
        }

        counter = 0;
        for (int i = n*(n-1); i < n*n; i++)
        {
            std::string upstream = "EAST_"+std::to_string(counter);
            std::string downstream = std::to_string(i);
            G->AddNode(upstream, n*linkLength, counter*linkLength);
            G->AddLink(upstream+"_"+downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
            G->AddLink(downstream+"_"+upstream, downstream, upstream, linkLength, makeSimpleCostMap(linkLength));
            counter++;
        }

        counter = 0;
        for (int i = n-1; i < n*n; i+=n)
        {
            std::string upstream = "NORTH_"+std::to_string(counter);
            std::string downstream = std::to_string(i);
            G->AddNode(upstream, counter*linkLength, n*linkLength);
            G->AddLink(upstream+"_"+downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
            G->AddLink(downstream+"_"+upstream, downstream, upstream, linkLength, makeSimpleCostMap(linkLength));
            counter++;
        }

        counter = 0;
        for (int i = 0; i < n*n; i+=n)
        {
            std::string upstream = "SOUTH_"+std::to_string(counter);
            std::string downstream = std::to_string(i);
            G->AddNode(upstream, counter*linkLength, -linkLength);
            G->AddLink(upstream+"_"+downstream, upstream, downstream, linkLength, makeSimpleCostMap(linkLength));
            G->AddLink(downstream+"_"+upstream, downstream, upstream, linkLength, makeSimpleCostMap(linkLength));
            counter++;
        }

        return G;
    }

} // namespace hipop
