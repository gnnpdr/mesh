#include "vertex_cluster.hpp"
#include "mesh_obj_converter.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    std::string filename = argv[1];
    OBJParser::OBJParser parser(filename);
    Mesh::Mesh mesh(parser);

    mesh.print();

    float detail_level = std::stof(argv[2]);
    VertexCluster::VertexCluster claster(mesh, detail_level);
    Mesh::Mesh new_mesh = claster.simplify();

    //new_mesh.print();

    MeshOBJConverter::MeshOBJConverter converter(new_mesh);
    converter.convert();

    return 0;
}