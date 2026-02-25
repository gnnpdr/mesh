#pragma once
#include "obj_parser.hpp"
#include <array>

namespace Mesh
{
class Mesh
{
    using Vertex = Vec3::Vec3f;
    using VertexInd = size_t;
    using Triangle = std::array<VertexInd, 3>;

    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;

public:

    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : vertices_(vertices), triangles_(triangles) {}
    Mesh(const OBJParser::OBJParser& parser)
    {
        auto& parser_vertices = parser.get_vertices();
        auto& parser_faces = parser.get_faces();

        for (const auto& v : parser_vertices)
        {
            vertices_.push_back(v);
        }

        for (const auto& f : parser_faces)
        {
            //вот здесь уже проходит триангуляция
            for (size_t i = 1; i + 1 < f.size(); i++) 
            {
               add_triangle(f[0], f[i], f[i + 1]);
            }
        }
    }

    const std::vector<Vertex>& get_vertices() const { return vertices_; }
    const std::vector<Triangle>& get_triangles() const { return triangles_; }
    
    size_t get_vert_amt() const { return vertices_.size(); }
    size_t get_triang_amt() const { return triangles_.size(); }

    void print()
    {
        std::cout << "vertices" << std::endl;
        for (auto v : vertices_)
        {
            std::cout << v.x() << " " << v.y() << " " << v.z() << std::endl; 
        }

        std::cout << "triangles" << std::endl;

        for (auto& t : triangles_)
        {
            std::cout << t[0] << " " << t[1] << " " << t[2] << std::endl; 
        }
    }

private:

    void add_vertice(Vertex& v)
    {
        vertices_.push_back(v);
    }

    void add_triangle(const size_t v1, const size_t v2, const size_t v3)
    {
        if (v1 >= vertices_.size() || v2 >= vertices_.size() || v3 >= vertices_.size()) 
        {
            std::cerr << "Error: Vertex index out of bounds\n";
            return;
        }

        if (v1 == v2 || v1 == v3 || v2 == v3) {
            std::cerr << "Error: Degenerate triangle (duplicate vertices)\n";
            return;
        }

        triangles_.push_back({v1, v2, v3});
    }

    void add_triangle(const Triangle& t)
    {
        VertexInd v1 = t.at(0);
        VertexInd v2 = t.at(1);
        VertexInd v3 = t.at(2);

        add_triangle(v1, v2, v3);
    }
};
}
