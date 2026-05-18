#include "algorithms/vertex_cluster.hpp"
#include "algorithms/edge_collapse.hpp"
#include "mesh/mesh_obj_converter.hpp"
#include "viewer/mesh_viewer.hpp"
#include "algorithms/metrics.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    if (argc == 2) 
    {
        std::string filename = argv[1];
        OBJParser::OBJParser parser(filename);
        Mesh::EdgeMesh mesh(parser);
        MeshViewer::MeshViewer viewer(mesh);
        viewer.start_viewer();
    }
    else
    {
        MeshViewer::MeshViewer viewer;
        viewer.start_viewer();
    }
    
    return 0;
}
