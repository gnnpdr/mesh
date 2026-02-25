#include "mesh.hpp"
#include <iostream>
#include <string>

int main() 
{
    std::string filename = "../box.obj";
    OBJParser::OBJParser parser(filename);
    Mesh::Mesh mesh(parser);

    mesh.print();

    return 0;
}