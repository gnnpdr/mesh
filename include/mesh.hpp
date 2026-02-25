#pragma once
#include "vec3.hpp"
#include <vector>
#include <array>

namespace Mesh
{
class Mesh
{
    using Vertice = Vec3::Vec3f;
    using VerticeInd = size_t;
    using Triangle = std::array<VerticeInd, 3>;

    std::vector<Vertice> vertices_;
    std::vector<Triangle> triangles_;

public:

    Mesh() = default;
    Mesh(const std::vector<Vertice>& vertices, const std::vector<std::array<VerticeInd, 3>>& triangles) : vertices_(vertices), triangles_(triangles) {}

    void add_vertice(Vertice& v)
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
        VerticeInd v1 = t.at(0);
        VerticeInd v2 = t.at(1);
        VerticeInd v3 = t.at(2);

        add_triangle(v1, v2, v3);
    }

    const std::vector<Vertice>& get_vertices() const { return vertices_; }
    const std::vector<Triangle>& get_triangles() const { return triangles_; }
    
    size_t get_vert_amt() const { return vertices_.size(); }
    size_t get_triang_amt() const { return triangles_.size(); }
};
}
