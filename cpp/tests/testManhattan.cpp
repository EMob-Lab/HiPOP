#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/create.h>

#include <memory>

using namespace hipop;


int testManhattan(int, char**)
{
    std::unique_ptr<OrientedGraph> G(makeManhattan(100, 10));
    assertTrue(static_cast<bool>(G), "Failed to create Manhattan graph");

    // FIXME Additional assertions to verify the structure of the Manhattan graph should be added here.

    return 0;
}
