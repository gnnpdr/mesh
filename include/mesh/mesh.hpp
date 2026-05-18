#pragma once
#include "mesh/obj_parser.hpp"
#include <array>
#include <set>
#include <map>
#include <iostream>

namespace Mesh
{

using Vertex = Vec3f;
using VertexInd = int;
using TriangleInd = int;
using Triangle = std::array<VertexInd, 3>;
using EdgeInd = int;

/**
 * @brief Mesh - class with vectors of vertices and triangles
 * 
 * Mesh can be created default, with triangles and vertices vectors or with OBJ parcer 
 * 
 * @param vertices_ - mesh vetices vector
 * @param triangles_ - mesh triangles vector
 * @param bounding_box_ - array with min and max coordinate (vec3) of 3D model
 */
class Mesh
{
protected:

    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;

    std::array<Vec3f, 2> bounding_box_;

public:

    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : vertices_(vertices), triangles_(triangles) 
    {
        bounding_box();
    }
    Mesh(const OBJParser::OBJParser& parser);

    virtual ~Mesh() = default;

    inline bool is_empty(){return vertices_.empty() || triangles_.empty();}

    inline const std::vector<Vertex>& get_vertices() const { return vertices_; }
    inline const std::vector<Triangle>& get_triangles() const { return triangles_; }
    
    inline size_t get_vert_amt() const { return vertices_.size(); }
    inline size_t get_triang_amt() const { return triangles_.size(); }

    virtual void print() const;

    inline std::array<Vec3f, 2> get_bounding_box() const { return bounding_box_; }
    std::array<float, 3> get_bounding_box_size() const;
    float get_bounding_box_diag_size() const;

    inline void set_vert(Vec3f& vert, VertexInd ind) { vertices_[ind] = vert; }
    inline void set_vertices(const std::vector<Vertex>& vertices) { vertices_ = vertices; }
    inline void set_triangle(Triangle& triangle, TriangleInd ind) { triangles_[ind] = triangle; }
    inline void set_triangle_vert(VertexInd vert_ind, TriangleInd ind, size_t vert_in_triangle_ind) { triangles_[ind][vert_in_triangle_ind] = vert_ind; }
    inline void set_triangles(std::vector<Triangle>& triangles) { triangles_ = triangles; }

private:

    void bounding_box();

    void add_triangle(const int v1, const int v2, const int v3)
    {
        if (v1 >= vertices_.size() || v2 >= vertices_.size() || v3 >= vertices_.size()) 
            throw std::out_of_range("Vertex index out of range");

        if (v1 == v2 || v1 == v3 || v2 == v3)
            throw std::invalid_argument("Degenerate triangle (duplicate vertices)");

        triangles_.push_back({v1, v2, v3});
    }

    inline void add_triangle(const Triangle& t)
    {
        VertexInd v1 = t.at(0);
        VertexInd v2 = t.at(1);
        VertexInd v3 = t.at(2);

        add_triangle(v1, v2, v3);
    }
};

/**
 * @brief Data about vertex
 * 
 * @param neighbor_vertices_ - verticex with the same edge (set for avoiding copy neighbor several times)
 * @param incident_triangles_ - triangles, using this vertex (triangles are individual, so we use vector, not set)
 * @param is_active_ - is this vertex used in not deleted edge
 */
struct VertexData
{
    std::set<VertexInd> neighbor_vertices_;
    std::vector<TriangleInd> incident_triangles_; 
    bool is_active_ = true;
};

/**
 * @brief Data about vertex
 * 
 * @param v1_, v2_ - vertices of this edge
 * @param t1_, t2_ - triangles, using this edge
 * @param is_active_ - is this edge used in not deleted edge
 */
struct Edge
{
    VertexInd v1_, v2_;
    TriangleInd t1_ = -1;
    TriangleInd t2_ = -1;
    bool is_active_ = true;

    Edge(VertexInd v1, VertexInd v2, TriangleInd t1) : v1_(v1), v2_(v2), t1_(t1) {}
};

class EdgeMesh : public Mesh
{
    std::vector<VertexData> vertex_data_;
    std::vector<Edge> edges_;

public:

    EdgeMesh() = default;

    explicit EdgeMesh(const Mesh&  base_mesh) : Mesh(base_mesh) { build_data(vertices_, triangles_); }
    explicit EdgeMesh(Mesh&& base_mesh) : Mesh(std::move(base_mesh)) { build_data(vertices_, triangles_); }

    EdgeMesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : Mesh()
    {
        vertices_ = std::move(vertices);
        triangles_ = std::move(triangles);
        build_data(vertices, triangles);
    }

    EdgeMesh(const OBJParser::OBJParser& parser) : Mesh(parser) { build_data(vertices_, triangles_); }
    ~EdgeMesh() = default;

    std::vector<VertexData>& get_vertex_data() { return vertex_data_; }

    std::vector<Edge>& get_edges() { return edges_; }
 
    void print() const override;

    EdgeInd find_edge(VertexInd v1, VertexInd v2) 
    {
        if (v1 > v2) std::swap(v1, v2);

        size_t edges_amt = edges_.size();
        for (EdgeInd e = 0; e < edges_amt; e++) 
        {
            if (edges_[e].is_active_ && edges_[e].v1_ == v1 && edges_[e].v2_ == v2)
                return e;
        }

        return -1;
    }

private:

    void collect_vertices_data(const std::vector<Vertex>& base_vertices, const std::vector<std::array<VertexInd, 3>>& base_triangles);
    void collect_edges(const std::vector<std::array<VertexInd, 3>>& base_triangles);
    void add_edge(VertexInd v1, VertexInd v2, TriangleInd t_ind, std::map<std::pair<VertexInd, VertexInd>, EdgeInd>& edge_map);

    inline void build_data(const std::vector<Vertex>& base_vertices, const std::vector<std::array<VertexInd, 3>>& base_triangles)
    {
        collect_vertices_data(base_vertices, base_triangles);
        collect_edges(base_triangles);
    }
};

template<typename Container, typename Getter>
std::array<Vec3f, 2> compute_bounding_box_of_group(const Container& container, Getter getter)
{
    float min_x = 1e30f, max_x = -1e30f;
    float min_y = 1e30f, max_y = -1e30f;
    float min_z = 1e30f, max_z = -1e30f;
    
    for (const auto& item : container) 
    {
        const auto& obj = getter(item); 
        auto bounding_box = obj.get_bounding_box();
        
        min_x = std::min(min_x, bounding_box[0].x());
        max_x = std::max(max_x, bounding_box[1].x());
        min_y = std::min(min_y, bounding_box[0].y());
        max_y = std::max(max_y, bounding_box[1].y());
        min_z = std::min(min_z, bounding_box[0].z());
        max_z = std::max(max_z, bounding_box[1].z());
    }
    
    
    Vec3f min_coords(min_x, min_y, min_z);
    Vec3f max_coords(max_x, max_y, max_z);

    std::array<Vec3f, 2> group_bounding_box{min_coords, max_coords};

    return group_bounding_box;
}

}