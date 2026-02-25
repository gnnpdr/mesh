#pragma once
#include "mesh.hpp"

#include <map>
#include <algorithm>

namespace VertexCluster
{
//разделяем на pos и data, потому что потом будем использовать map
struct CellPos
{
    //адрес ячейки по трем осям
    int x_, y_, z_;
    CellPos(int x, int y, int z) : x_(x), y_(y), z_(z) {}

    bool operator<(const CellPos& other) const 
    {
        if (x_ != other.x_) 
            return x_ < other.x_;
        if (y_ != other.y_) 
            return y_ < other.y_;
        return z_ < other.z_;
    }
};

class CellData
{
    Vec3::Vec3f sum_;
    size_t verteces_amt_ = 0;

public:

    CellData() = default;

    void add(const Vec3::Vec3f& v)
    {
        sum_ = sum_ + v;      //можно вообще добавить оператор += в vec3
        verteces_amt_++;
    }

    Vec3::Vec3f calc_average_v()
    {
        if (!verteces_amt_) 
            return Vec3::Vec3f(0, 0, 0);

        return Vec3::Vec3f(sum_.x() / verteces_amt_, sum_.y() / verteces_amt_, sum_.z() / verteces_amt_);
    }
};

namespace Detail
{
    float DETAIL_LEVEL = 0.5;
}

class VertexCluster
{
    Mesh::Mesh input_mesh_;

    std::map<CellPos, CellData> cells_;
    std::vector<size_t> old_to_new_cells_;

    std::map<CellPos, size_t> cell_to_new_ind_;
    std::vector<Vec3::Vec3f> new_vertices_;

    std::vector<std::array<size_t, 3>> new_faces_;

    Vec3::Vec3f min_bound_;
    Vec3::Vec3f max_bound_;

    float cell_size_;

public:

    VertexCluster(const Mesh::Mesh& mesh) : input_mesh_(mesh)
    {
        find_bounds(mesh);

        float max_model_size = std::max({max_bound_.x() - min_bound_.x(), max_bound_.y() - min_bound_.y(), max_bound_.z() - min_bound_.z()});
        cell_size_ = max_model_size * Detail::DETAIL_LEVEL;

        size_t vertices_amt = input_mesh_.get_vertices().size();
        old_to_new_cells_.resize(vertices_amt);
    } 

    Mesh::Mesh simplify() 
    {   
        cluster_vertices();

        calc_new_vertices();
    
        old_new_v_match();
        
        triangulate();
        
        return Mesh::Mesh(new_vertices_, new_faces_);
    }

private:

    void find_bounds(const Mesh::Mesh& input_mesh)
    {
        auto& vertices = input_mesh.get_vertices();
        if (vertices.empty()) return;

        float min_x = 0;
        float min_y = 0;
        float min_z = 0;

        float max_x = 0;
        float max_y = 0;
        float max_z = 0;

        for (const auto& v : vertices) 
        {
            min_x = std::min(min_x, v.x());
            min_y = std::min(min_y, v.y());
            min_z = std::min(min_z, v.z());
            max_x = std::max(max_x, v.x());
            max_y = std::max(max_y, v.y());
            max_z = std::max(max_z, v.z());
        }

        min_bound_ = Vec3::Vec3f(min_x, min_y, min_z);
        max_bound_ = Vec3::Vec3f(max_x, max_y, max_z);

        std::cout << "Bounds: min(" << min_x << "," << min_y << "," << min_z 
              << ") max(" << max_x << "," << max_y << "," << max_z << ")\n";
    }

    CellPos get_cell_pos(const Vec3::Vec3f& v) 
    {
        return CellPos(floor((v.x() - min_bound_.x()) / cell_size_), floor((v.y() - min_bound_.y()) / cell_size_), floor((v.z() - min_bound_.z()) / cell_size_));
    }

    void cluster_vertices()
    {
        auto& vertices = input_mesh_.get_vertices();
        size_t vertices_amt = vertices.size();
        for (size_t i = 0; i < vertices_amt; i++) 
        {
            CellPos cell_pos = get_cell_pos(vertices[i]);
            cells_[cell_pos].add(vertices[i]);
        }
    }

    void calc_new_vertices()
    {
        for (auto& [pos, data] : cells_) 
        {
            cell_to_new_ind_[pos] = new_vertices_.size();
            Vec3::Vec3f average_v = data.calc_average_v();
            new_vertices_.push_back(average_v);
        }
    }

    void old_new_v_match()
    {
        auto& vertices = input_mesh_.get_vertices();
        size_t vertices_amt = vertices.size();
        for (size_t v = 0; v < vertices_amt; v++) 
        {
            CellPos cell_pos = get_cell_pos(vertices[v]);
            old_to_new_cells_[v] = cell_to_new_ind_[cell_pos];
        }
    }

    void triangulate()
    {
        auto& faces = input_mesh_.get_triangles();
        int degenerateCount = 0;
        
        for (const auto& face : faces) 
        {
            size_t v0 = old_to_new_cells_[face[0]];
            size_t v1 = old_to_new_cells_[face[1]];
            size_t v2 = old_to_new_cells_[face[2]];
            
            //проверяем, в порядке ли треугольник
            if (v0 != v1 && v1 != v2 && v0 != v2) 
                new_faces_.push_back({v0, v1, v2});
            else
                degenerateCount++;
        }
    }
};
}