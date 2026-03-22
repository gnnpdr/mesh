#pragma once
#include "obj_parser.hpp"
#include <array>
#include <set>
#include <map>

namespace Mesh
{

using Vertex = Vec3::Vec3f;
using VertexInd = size_t;
using TriangleInd = size_t;
using Triangle = std::array<VertexInd, 3>;
using EdgeInd = size_t;


class Mesh
{
protected:

    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;

    std::array<Vec3::Vec3f, 2> bounding_box_;

public:

    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : vertices_(vertices), triangles_(triangles) 
    {
        bounding_box();
    }

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

        bounding_box();
    }

    const std::vector<Vertex>& get_vertices() const { return vertices_; }
    const std::vector<Triangle>& get_triangles() const { return triangles_; }
    
    size_t get_vert_amt() const { return vertices_.size(); }
    size_t get_triang_amt() const { return triangles_.size(); }

    virtual void print() const
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

    std::array<Vec3::Vec3f, 2> get_bounding_box() const 
    {
        return bounding_box_;
    }

    std::array<float, 3> get_bounding_box_size() const 
    {
        Vec3::Vec3f min_bound = bounding_box_[0];
        Vec3::Vec3f max_bound = bounding_box_[1];

        float x = max_bound.x() - min_bound.x();
        float y = max_bound.y() - min_bound.y();
        float z = max_bound.z() - min_bound.z();

        return {x, y, z};
    }

    float get_bounding_box_diag_size() const
    {
        Vec3::Vec3f min_bound = bounding_box_[0];
        Vec3::Vec3f max_bound = bounding_box_[1];

        size_t x = max_bound.x() - min_bound.x();
        size_t y = max_bound.y() - min_bound.y();
        size_t z = max_bound.z() - min_bound.z();

        return sqrt(x * x + y * y + z * z);
    }

    virtual ~Mesh() = default;

private:

    void bounding_box()
    {
        if (vertices_.empty()) return;

        float min_x = vertices_[0].x();
        float min_y = vertices_[0].y();
        float min_z = vertices_[0].z();

        float max_x = vertices_[0].x();
        float max_y = vertices_[0].y();
        float max_z = vertices_[0].z();

        for (const auto& v : vertices_) 
        {
            min_x = std::min(min_x, v.x());
            min_y = std::min(min_y, v.y());
            min_z = std::min(min_z, v.z());

            max_x = std::max(max_x, v.x());
            max_y = std::max(max_y, v.y());
            max_z = std::max(max_z, v.z());
        }

        bounding_box_[0] = Vec3::Vec3f(min_x, min_y, min_z);
        bounding_box_[1] = Vec3::Vec3f(max_x, max_y, max_z);
    }

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

struct VertexData
{
    std::set<VertexInd> neighbor_vertices_;     //мы избегаем копирование соседей, потому что проходя по треугольникам нам может много раз попасться одна вершина, используем сет
    std::vector<TriangleInd> incident_triangles_; //с треугольниками не так, они индивидуальны, достаточно вектора
    bool is_active = true;
};

struct Edge
{
    VertexInd v1_, v2_;
    TriangleInd t1_ = -1;
    TriangleInd t2_ = -1;
    float cost_;
    bool is_active = true;

    Edge(VertexInd v1, VertexInd v2, TriangleInd t1) : v1_(v1), v2_(v2), t1_(t1) {}
};

class EdgeMesh : public Mesh
{
    std::vector<VertexData> vertex_data_;
    std::vector<Edge> edges_;

public:

    EdgeMesh() = default;

    explicit EdgeMesh(const Mesh&  base_mesh) : Mesh(base_mesh)
    {
        build_data(vertices_, triangles_);
    }

    explicit EdgeMesh(Mesh&& base_mesh) : Mesh(std::move(base_mesh))
    {
        build_data(vertices_, triangles_);
    }

    EdgeMesh(const std::vector<Vertex>& vertices, const std::vector<std::array<VertexInd, 3>>& triangles) : Mesh()
    {
        vertices_ = std::move(vertices);
        triangles_ = std::move(triangles);
        build_data(vertices, triangles);
    }

    EdgeMesh(const OBJParser::OBJParser& parser) : Mesh(parser)
    {
        build_data(vertices_, triangles_);
    }

    ~EdgeMesh() = default;

    void print() const override
    {
        std::cout << "vertices" << std::endl;
        for (auto v : vertices_)
        {
            std::cout << v.x() << " " << v.y() << " " << v.z() << std::endl; 
        }

        std::cout << "triangles" << std::endl;

        for (const auto& t : triangles_)
        {
            std::cout << t[0] << " " << t[1] << " " << t[2] << std::endl; 
        }

        std::cout << "vertices data" << std::endl;

        for (auto& vd : vertex_data_)
        {
            std::cout << "neighbor vertices" << std::endl;
            const auto& nv = vd.neighbor_vertices_;
            for (const auto v : nv)
            {
                std::cout << v << " ";
            }
            std::cout << std::endl;

            std::cout << "incident triangles" << std::endl;
            const auto& it = vd.incident_triangles_;
            for (const auto t : it)
            {
                std::cout << t << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "edges" << std::endl;

        for (const auto& e : edges_)
        {
            std::cout << "v1 " << e.v1_ << " v2 " << e.v2_ << " t1 " << e.t1_ << " t2 " << e.t2_ <<  std::endl;
        }
    }

private:

    void collect_vertices_data(const std::vector<Vertex>& base_vertices, const std::vector<std::array<VertexInd, 3>>& base_triangles)
    {
        size_t v_amt = base_vertices.size();
        vertex_data_.resize(v_amt);

        size_t t_amt = base_triangles.size();
        for (size_t t_ind = 0; t_ind < t_amt; t_ind++)
        {
            const auto& t = base_triangles[t_ind];
            for (size_t v = 0; v < 3; v++)
            {
                size_t v_ind = t[v];
                vertex_data_[v_ind].incident_triangles_.push_back(t_ind);

                size_t next_v = t[(v + 1) % 3];
                size_t prev_v = t[(v + 2) % 3];
                vertex_data_[v_ind].neighbor_vertices_.insert(next_v);
                vertex_data_[v_ind].neighbor_vertices_.insert(prev_v);
            }
        }
    }

    void collect_edges(const std::vector<std::array<VertexInd, 3>>& base_triangles)
    {
        size_t t_amt = base_triangles.size();
        std::map<std::pair<VertexInd, VertexInd>, EdgeInd> edge_map;
        for (size_t t_ind = 0; t_ind < t_amt; t_ind++)
        {
            const auto& t = base_triangles[t_ind];
            add_edge(t[0], t[1], t_ind, edge_map);
            add_edge(t[1], t[2], t_ind, edge_map);
            add_edge(t[2], t[0], t_ind, edge_map);
        }
    }

    void add_edge(VertexInd v1, VertexInd v2, TriangleInd t_ind, std::map<std::pair<VertexInd, VertexInd>, EdgeInd>& edge_map)
    {
        if (v1 > v2)
            std::swap(v1, v2);

        auto edge = edge_map.find({v1, v2});
        if (edge == edge_map.end())
        {
            Edge new_edge(v1, v2, t_ind);
            edges_.push_back(new_edge);
            edge_map[{v1, v2}] = edges_.size() - 1;    
        }
        else
        {
            edges_[edge->second].t2_ = t_ind;
        }
    }

    void build_data(const std::vector<Vertex>& base_vertices, const std::vector<std::array<VertexInd, 3>>& base_triangles)
    {
        collect_vertices_data(base_vertices, base_triangles);
        collect_edges(base_triangles);
    }
};
}