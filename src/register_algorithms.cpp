#include "algorithms/adapters.hpp"
#include "algorithms/simplifier_registry.hpp"

namespace MeshSimplify 
{

REGISTER_SIMPLIFIER(VertexClusterAdapter, "Vertex Cluster")
REGISTER_SIMPLIFIER(EdgeCollapseAdapter, "Edge Collapse")
REGISTER_SIMPLIFIER(QuadricAdapter, "Quadric")
// REGISTER_SIMPLIFIER(MyCoolAlgorithmAdapter, "My Cool Algorithm")

}