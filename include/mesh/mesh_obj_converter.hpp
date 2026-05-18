#pragma once
#include "mesh/mesh.hpp"
#include <fstream>

namespace MeshOBJConverter
{

class MeshOBJConverter
{
    Mesh::Mesh mesh_;

public:

    MeshOBJConverter(const Mesh::Mesh& mesh) : mesh_(mesh) {}

    void convert()
    {
        std::ofstream file("new_mesh.obj");
        if (!file.is_open()) 
            throw std::runtime_error("Failed to open file: new_mesh.obj");

        file << "#woooow, your mesh is sooo big\n#but i think i can handle it\n" << std::endl;

        auto& vertices = mesh_.get_vertices();
        file << "#sir, your vertices, sir" << std::endl;
        for (const auto& v : vertices)
            file << "v " << v.x() << " " << v.y() << " " << v.z() << std::endl;
        
        file << std::endl;
        auto& tringles = mesh_.get_triangles();
        file << "#sir, your triangles, sir" << std::endl;
        for (const auto& t : tringles)
            file << "f " << t[0] + 1 << " " <<  t[1] + 1 << " " << t[2] + 1 << std::endl;

        file << std::endl;
        file << "#fooh, it was difficult, sir, im breathless now" << std::endl;

        file.close(); 
    }
};
}