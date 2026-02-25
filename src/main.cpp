#include "vertex_cluster.hpp"
#include <iostream>
#include <string>

int main() 
{
    std::string filename = "../test.obj";
    OBJParser::OBJParser parser(filename);
    Mesh::Mesh mesh(parser);

    //mesh.print();

    VertexCluster::VertexCluster claster(mesh);
    Mesh::Mesh new_mesh = claster.simplify();

    new_mesh.print();

    return 0;
}