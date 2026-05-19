#include "algorithms/vertex_cluster.hpp"
#include <iostream>
#include <stdexcept>

Mesh::Mesh VertexCluster::VertexCluster::simplify() 
{   
    if (input_mesh_.is_empty())
        throw std::runtime_error("VertexCluster: input mesh is empty");
    
    cluster_vertices();
    
    calc_new_vertices();
    
    old_new_v_match();
    
    triangulate();
    
    Mesh::Mesh result(new_vertices_, new_faces_);
    
    return result;
}

void VertexCluster::VertexCluster::cluster_vertices()
{
    const auto& vertices = input_mesh_.get_vertices();
    if (vertices.empty()) 
        throw std::runtime_error("VertexCluster: input mesh has no vertices");
    
    cells_.clear();
    
    size_t vertices_amt = vertices.size();
    for (size_t v = 0; v < vertices_amt; v++) 
    {
        CellPos cell_pos = get_cell_pos(vertices[v]);
        cells_[cell_pos].add(vertices[v]);
    }
}

void VertexCluster::VertexCluster::calc_new_vertices()
{
    if (cells_.empty())
        throw std::runtime_error("VertexCluster: no cells to process");
    
    cell_to_new_ind_.clear();
    new_vertices_.clear();
    new_vertices_.reserve(cells_.size());
    
    for (auto& [pos, data] : cells_) 
    {
        cell_to_new_ind_[pos] = new_vertices_.size();
        Vec3f average_v = data.calc_average_v();
        new_vertices_.push_back(average_v);
    }
}

void VertexCluster::VertexCluster::old_new_v_match()
{
    const auto& vertices = input_mesh_.get_vertices();
    size_t vertices_amt = vertices.size();
    
    if (old_to_new_v_.size() != vertices_amt)
        old_to_new_v_.resize(vertices_amt);
    
    int unmatched_count = 0;
    
    for (size_t v = 0; v < vertices_amt; v++) 
    {
        CellPos cell_pos = get_cell_pos(vertices[v]);
        auto it = cell_to_new_ind_.find(cell_pos);
        
        if (it != cell_to_new_ind_.end()) 
            old_to_new_v_[v] = it->second;
        else
        {
            old_to_new_v_[v] = -1;
            unmatched_count++;
        }
    }
    
    if (unmatched_count > 0)
        std::cerr << "Warning: " << unmatched_count << " vertices not mapped to any cell" << std::endl;
}

void VertexCluster::VertexCluster::triangulate()
{
    const auto& faces = input_mesh_.get_triangles();
    if (faces.empty())
        throw std::runtime_error("VertexCluster: input mesh has no triangles");
    
    new_faces_.clear();
    new_faces_.reserve(faces.size());
    
    int degenerate_count = 0;
    int invalid_index_count = 0;
    
    for (const auto& face : faces) 
    {
        if (face[0] < 0 || face[0] >= old_to_new_v_.size() ||
            face[1] < 0 || face[1] >= old_to_new_v_.size() ||
            face[2] < 0 || face[2] >= old_to_new_v_.size()) {
            invalid_index_count++;
            continue;
        }
        
        int v0 = old_to_new_v_[face[0]];
        int v1 = old_to_new_v_[face[1]];
        int v2 = old_to_new_v_[face[2]];
        
        if (v0 != v1 && v1 != v2 && v0 != v2) 
        {
            if (v0 >= 0 && v1 >= 0 && v2 >= 0 && 
                v0 < new_vertices_.size() &&
                v1 < new_vertices_.size() &&
                v2 < new_vertices_.size()) 
                new_faces_.push_back({v0, v1, v2});
            else
                invalid_index_count++;
        } 
        else 
            degenerate_count++;
    }
    
    if (new_faces_.empty()) 
        throw std::runtime_error("VertexCluster: simplification produced no triangles");
}