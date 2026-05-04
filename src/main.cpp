#include "vertex_cluster.hpp"
#include "edge_collapse.hpp"
#include "mesh_obj_converter.hpp"
#include "mesh_viewer.hpp"
#include "metrics.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    std::string filename = argv[1];
    
    OBJParser::OBJParser parser(filename);
    Mesh::EdgeMesh mesh(parser);

    MeshViewer::MeshViewer viewer(mesh);
    viewer.show_mesh();

    return 0;
}

