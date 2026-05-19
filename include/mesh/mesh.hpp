/**
 * @file mesh.hpp
 * @brief Cетка и вспомогательные структуры (вершины и ребра)
 */

#pragma once

#include "mesh/obj_parser.hpp"
#include <array>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace Mesh
{

using Vertex = Vec3f;          
using VertexInd = int;      
using TriangleInd = int;        
using Triangle = std::array<VertexInd, 3>;
using EdgeInd = int;        

/**
 * @brief Базовый сетки
 * @note Индексация вершин и треугольников начинается с 0
 */
class Mesh
{
protected:
    std::vector<Vertex> vertices_;   ///< Список вершин
    std::vector<Triangle> triangles_; ///< Список треугольников
    std::array<Vec3f, 2> bounding_box_; ///< [min, max]

public:
    Mesh() = default;
    
    Mesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : vertices_(vertices), triangles_(triangles) 
    {
        if (vertices_.empty() || triangles_.empty()) 
            throw std::invalid_argument("Mesh: vertices and triangles must not be empty");
        
        for (const auto& tri : triangles_) 
        { 
            for (int idx : tri) 
            {
                if (idx < 0 || idx >= vertices_.size())
                    throw std::out_of_range("Mesh: triangle vertex index out of range");
            }
        }
        
        bounding_box();
    }
    
    Mesh(const OBJParser::OBJParser& parser);
    
    virtual ~Mesh() = default;

    inline bool is_empty() const { return vertices_.empty() || triangles_.empty(); }
    
    inline const std::vector<Vertex>& get_vertices() const { return vertices_; }
    
    inline const std::vector<Triangle>& get_triangles() const { return triangles_; }
    
    inline size_t get_vert_amt() const { return vertices_.size(); }
    
    inline size_t get_triang_amt() const { return triangles_.size(); }

    virtual void print() const;

    inline std::array<Vec3f, 2> get_bounding_box() const { return bounding_box_; }
    
    std::array<float, 3> get_bounding_box_size() const;
    
    float get_bounding_box_diag_size() const;

    inline void set_vert(const Vec3f& vert, VertexInd ind) 
    { 
        if (ind < 0 || ind >= vertices_.size()) 
            throw std::out_of_range("set_vert: vertex index out of range");
    
        vertices_[ind] = vert; 
    }
    
    inline void set_vertices(const std::vector<Vertex>& vertices) 
    { 
        vertices_ = vertices; 
        bounding_box();
    }
    
    inline void set_triangle(const Triangle& triangle, TriangleInd ind) 
    { 
        if (ind < 0 || ind >= triangles_.size())
            throw std::out_of_range("set_triangle: triangle index out of range");

        triangles_[ind] = triangle; 
    }
    
    inline void set_triangle_vert(VertexInd vert_ind, TriangleInd ind, size_t vert_in_triangle_ind) 
    { 
        if (ind < 0 || ind >= triangles_.size())
            throw std::out_of_range("set_triangle_vert: triangle index out of range");
    
        if (vert_in_triangle_ind >= 3)
            throw std::out_of_range("set_triangle_vert: vertex position in triangle must be 0,1,2");
    
        triangles_[ind][vert_in_triangle_ind] = vert_ind; 
    }
    
    inline void set_triangles(const std::vector<Triangle>& triangles) { triangles_ = triangles; }

private:
    void bounding_box();

    /**
     * @brief Добавляет треугольник в сетку
     * @throws std::out_of_range Если индексы вне диапазона
     * @throws std::invalid_argument Если треугольник вырожден
     */
    void add_triangle(const int v1, const int v2, const int v3)
    {
        if (v1 >= vertices_.size() || 
            v2 >= vertices_.size() || 
            v3 >= vertices_.size()) 
            throw std::out_of_range("Vertex index out of range");
        
        if (v1 == v2 || v1 == v3 || v2 == v3) 
            throw std::invalid_argument("Degenerate triangle (duplicate vertices)");

        triangles_.push_back({v1, v2, v3});
    }

    inline void add_triangle(const Triangle& t){ add_triangle(t[0], t[1], t[2]);}
};

struct VertexData
{
    std::set<VertexInd> neighbor_vertices_;      ///< Соседние вершины (по рёбрам)
    std::vector<TriangleInd> incident_triangles_; ///< Инцидентные треугольники
    bool is_active_ = true;                       ///< Активна ли вершина (не удалена)
};

/**
 * @brief Ребро сетки для алгоритмов схлопывания
 * 
 * Если t2_ == -1, ребро граничное (принадлежит только одному треугольнику)
 */
struct Edge
{
    VertexInd v1_, v2_;        
    TriangleInd t1_ = -1;   
    TriangleInd t2_ = -1; 
    bool is_active_ = true;  
    Edge(VertexInd v1, VertexInd v2, TriangleInd t1) : v1_(v1), v2_(v2), t1_(t1) {}
};

/**
 * @brief Расширенная сетка с топологической информацией
 * 
 * Содержит данные о связях между вершинами, рёбрами и треугольниками.
 * Необходима для алгоритмов Edge Collapse и Quadric.
 * 
 * @note Индексы в vertex_data_ соответствуют индексам в vertices_
 */
class EdgeMesh : public Mesh
{
    std::vector<VertexData> vertex_data_;  ///< Данные о вершинах
    std::vector<Edge> edges_;              ///< Список рёбер

public:
    EdgeMesh() = default;

    explicit EdgeMesh(const Mesh& base_mesh) : Mesh(base_mesh) { build_data(vertices_, triangles_); }
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
    
    const std::vector<VertexData>& get_vertex_data() const { return vertex_data_; }

    std::vector<Edge>& get_edges() { return edges_; }
    
    const std::vector<Edge>& get_edges() const { return edges_; }
 
    void print() const override;

    EdgeInd find_edge(VertexInd v1, VertexInd v2) 
    {
        if (v1 > v2) std::swap(v1, v2);

        for (EdgeInd e = 0; e < edges_.size(); e++) 
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
    if (container.empty()) 
        throw std::invalid_argument("compute_bounding_box_of_group: container is empty");
    
    float min_x = std::numeric_limits<float>::max(), max_x = -std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max(), max_y = -std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max(), max_z = -std::numeric_limits<float>::max();
    
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

    return {min_coords, max_coords};
}

}