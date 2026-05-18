#include "algorithms/vertex_cluster.hpp"

Mesh::Mesh VertexCluster::VertexCluster::simplify() 
{   
    cluster_vertices();
    
    calc_new_vertices();

    old_new_v_match();
    
    triangulate();
    
    return Mesh::Mesh(new_vertices_, new_faces_);
}

void VertexCluster::VertexCluster::cluster_vertices()
{
    auto& vertices = input_mesh_.get_vertices();
    size_t vertices_amt = vertices.size();
    for (size_t v = 0; v < vertices_amt; v++) 
    {
        CellPos cell_pos = get_cell_pos(vertices[v]);
        cells_[cell_pos].add(vertices[v]);
    }
}

void VertexCluster::VertexCluster::calc_new_vertices()
{
    for (auto& [pos, data] : cells_) 
    {
        cell_to_new_ind_[pos] = new_vertices_.size();
        Vec3f average_v = data.calc_average_v();
        new_vertices_.push_back(average_v);
    }
}

void VertexCluster::VertexCluster::old_new_v_match()
{
    auto& vertices = input_mesh_.get_vertices();
    size_t vertices_amt = vertices.size();
    for (size_t v = 0; v < vertices_amt; v++) 
    {
        CellPos cell_pos = get_cell_pos(vertices[v]);
        old_to_new_v_[v] = cell_to_new_ind_[cell_pos];
    }
}

void VertexCluster::VertexCluster::triangulate()
{
    auto& faces = input_mesh_.get_triangles();
    int degenerateCount = 0;
    
    for (const auto& face : faces) 
    {
        int v0 = old_to_new_v_[face[0]];
        int v1 = old_to_new_v_[face[1]];
        int v2 = old_to_new_v_[face[2]];
        
        if (v0 != v1 && v1 != v2 && v0 != v2) 
            new_faces_.push_back({v0, v1, v2});
        else
            degenerateCount++;
    }
}