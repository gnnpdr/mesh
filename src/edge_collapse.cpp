#include "algorithms/edge_collapse.hpp"
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <array>
#include <stdexcept>

void EdgeCollapse::EdgeCollapse::build_heap() 
{
    auto& edges = mesh_.get_edges();
    size_t edges_amt = edges.size();
    for (EdgeInd e = 0; e < edges_amt; e++) 
    {
        if (edges[e].is_active_) 
        {
            float cost = calc_edge_cost(e);
            edge_heap_.push({e, cost});
        }
    }
}

void EdgeCollapse::EdgeCollapse::remove_degenerate_triangles() 
{
    for (auto& vd : mesh_.get_vertex_data())
        vd.incident_triangles_.clear();

    std::vector<std::array<VertexInd, 3>> new_triangles;
    auto& triangles = mesh_.get_triangles();

    for (const auto& triangle : triangles)
    {
        if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[2] == triangle[0])
            continue;

        TriangleInd new_ind = new_triangles.size();
        new_triangles.push_back(triangle);

        for (int i = 0; i < 3; i++) 
        {
            if (triangle[i] >= 0 && triangle[i] < mesh_.get_vertex_data().size())
                mesh_.get_vertex_data()[triangle[i]].incident_triangles_.push_back(new_ind);
        }
    }

    mesh_.set_triangles(new_triangles);
}

Mesh::EdgeMesh& EdgeCollapse::EdgeCollapse::simplify() 
{   
    if (final_triangles_amt_ >= mesh_.get_triang_amt()) 
    {
        std::cout << "Target triangle count already reached, skipping simplification" << std::endl;
        return mesh_;
    }
    
    size_t last_count = mesh_.get_triangles().size();
    int iteration = 0;
    
    while (mesh_.get_triangles().size() > final_triangles_amt_ && !edge_heap_.empty()) 
    {
        EdgePtr best = get_best_edge();
        if (best.edge_ind_ == Detail::ERROR_VAL) break;

        collapse_edge(best.edge_ind_);
        update_edges_in_heap(best.edge_ind_);
        iteration++;
    }

    return mesh_;
}

EdgeCollapse::EdgePtr EdgeCollapse::EdgeCollapse::get_best_edge() 
{
    while (!edge_heap_.empty()) 
    {
        EdgePtr top = edge_heap_.top();
        
        if (top.edge_ind_ < 0 || top.edge_ind_ >= mesh_.get_edges().size()) 
        {
            edge_heap_.pop();
            continue;
        }
        
        const Mesh::Edge& edge = mesh_.get_edges()[top.edge_ind_];
        
        if (!edge.is_active_) 
        {
            edge_heap_.pop();
            continue;
        }
        
        float current_cost = calc_edge_cost(top.edge_ind_);
        if (std::abs(current_cost - top.cost_) > 1e-6f) 
        {
            edge_heap_.pop();
            continue;
        }
        
        return top;
    }
    return {Detail::ERROR_VAL, 0.0f};
}

void EdgeCollapse::EdgeCollapse::update_edges_in_heap(EdgeInd collapsed_edge_ind) 
{
    auto& edges = mesh_.get_edges();
    if (collapsed_edge_ind < 0 || collapsed_edge_ind >= edges.size())
        return;
    
    const Mesh::Edge& collapsed_edge = edges[collapsed_edge_ind];
    VertexInd v = collapsed_edge.v1_;

    const auto& neighbors = mesh_.get_vertex_data()[v].neighbor_vertices_;

    for (VertexInd neighbor : neighbors) 
    {
        EdgeInd e_ind = mesh_.find_edge(v, neighbor);
        if (e_ind == -1) continue;

        float new_cost = calc_edge_cost(e_ind);
        edge_heap_.push({e_ind, new_cost});
    }

    if (edge_heap_.size() > 1000000) 
    {
        std::cout << "Heap too large (" << edge_heap_.size() << "), rebuilding..." << std::endl;
        rebuild_heap();
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void EdgeCollapse::SimpleEdgeCollapse::collapse_edge(EdgeInd edge_ind)
{
    auto& vertices = mesh_.get_vertices();
    auto& triangles = mesh_.get_triangles();
    Mesh::Edge& edge_to_remove = mesh_.get_edges()[edge_ind];

    VertexInd vert_to_keep = edge_to_remove.v1_;
    VertexInd vert_to_remove = edge_to_remove.v2_;

    if (vert_to_keep >= vertices.size() || vert_to_remove >= vertices.size()) 
        throw std::out_of_range("SimpleEdgeCollapse: vertex index out of range");

    Vec3f new_pos = calc_new_pos(vert_to_keep, vert_to_remove);
    mesh_.set_vert(new_pos, vert_to_keep);

    auto& triangles_to_update = mesh_.get_vertex_data()[vert_to_remove].incident_triangles_;  
    for (TriangleInd triangle_ind : triangles_to_update) 
    {
        if (triangle_ind < 0 || triangle_ind >= triangles.size())
            continue;

        auto& triangle = triangles[triangle_ind];
        for (int i = 0; i < 3; i++) 
        {
            if (triangle[i] == vert_to_remove) 
                mesh_.set_triangle_vert(vert_to_keep, triangle_ind, i);
        }
    }

    auto& keep_neighbors = mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_;
    auto& remove_neighbors = mesh_.get_vertex_data()[vert_to_remove].neighbor_vertices_;

    for (VertexInd neighbor : remove_neighbors) 
    {
        if (neighbor != vert_to_keep) 
        {
            keep_neighbors.insert(neighbor);
            auto& nb_neighbors = mesh_.get_vertex_data()[neighbor].neighbor_vertices_;
            nb_neighbors.erase(vert_to_remove);
            nb_neighbors.insert(vert_to_keep);
        }
    }
    keep_neighbors.erase(vert_to_keep); 

    mesh_.get_vertex_data()[vert_to_remove].is_active_ = false;
    edge_to_remove.is_active_ = false;

    remove_degenerate_triangles();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------

Vec3f EdgeCollapse::QuadricEdgeCollapse::calc_new_pos(VertexInd v1, VertexInd v2)
{
    Eigen::Matrix4f Q = vertex_Qs_[v1] + vertex_Qs_[v2];

    if (has_nan(Q) || is_degenerate(Q))
        return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;

    Eigen::Matrix3f A = Q.topLeftCorner<3,3>();
    Eigen::Vector3f B = -Q.block<3,1>(0,3);

    Eigen::Vector3f optimal_pos = A.ldlt().solve(B);

    if (optimal_pos.hasNaN())
        return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;

    Vec3f result(optimal_pos.x(), optimal_pos.y(), optimal_pos.z());

    float max_dist = 1000.0f;
    if (result.norm() > max_dist)
        return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;

    return result;
}

float EdgeCollapse::QuadricEdgeCollapse::calc_edge_cost(EdgeInd e) 
{ 
    const auto& edge = mesh_.get_edges()[e];
    auto v1 = edge.v1_;
    auto v2 = edge.v2_;

    Eigen::Matrix4f sum_Q = vertex_Qs_[v1] + vertex_Qs_[v2];

    if (has_nan(sum_Q) || is_degenerate(sum_Q)) 
        return Vec3::distance(mesh_.get_vertices()[v1], mesh_.get_vertices()[v2]);

    Vec3f optimal_pos = calc_new_pos(v1, v2);
    float error = evaluate_error(sum_Q, optimal_pos);

    if (std::isnan(error) || std::isinf(error))
        return Vec3::distance(mesh_.get_vertices()[v1], mesh_.get_vertices()[v2]);

    return error;
}

void EdgeCollapse::QuadricEdgeCollapse::collapse_edge(EdgeInd edge_ind)
{
    auto& vertices = mesh_.get_vertices();
    auto& triangles = mesh_.get_triangles();
    Mesh::Edge& edge_to_remove = mesh_.get_edges()[edge_ind];

    VertexInd vert_to_keep = edge_to_remove.v1_;
    VertexInd vert_to_remove = edge_to_remove.v2_;

    Vec3f new_pos = calc_new_pos(vert_to_keep, vert_to_remove);
    mesh_.set_vert(new_pos, vert_to_keep);

    vertex_Qs_[vert_to_keep] = vertex_Qs_[vert_to_keep] + vertex_Qs_[vert_to_remove];

    auto& triangles_to_update = mesh_.get_vertex_data()[vert_to_remove].incident_triangles_;  
    for (TriangleInd triangle_ind : triangles_to_update) 
    {
        auto& triangle = triangles[triangle_ind];
        for (int i = 0; i < 3; i++) 
        {
            if (triangle[i] == vert_to_remove)
                mesh_.set_triangle_vert(vert_to_keep, triangle_ind, i);
        }
    }

    std::set<VertexInd> vertices_to_update;
    vertices_to_update.insert(vert_to_keep);
    for (VertexInd neighbor : mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_)
        vertices_to_update.insert(neighbor);

    for (VertexInd v : vertices_to_update) 
        update_quadrics_around_vertex(v);
    
    auto& keep_neighbors = mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_;
    auto& remove_neighbors = mesh_.get_vertex_data()[vert_to_remove].neighbor_vertices_;

    for (VertexInd neighbor : remove_neighbors) 
    {
        if (neighbor != vert_to_keep) 
        {
            keep_neighbors.insert(neighbor);
            auto& nb_neighbors = mesh_.get_vertex_data()[neighbor].neighbor_vertices_;
            nb_neighbors.erase(vert_to_remove);
            nb_neighbors.insert(vert_to_keep);
        }
    }
    keep_neighbors.erase(vert_to_keep);

    mesh_.get_vertex_data()[vert_to_remove].is_active_ = false;
    edge_to_remove.is_active_ = false;

    remove_degenerate_triangles();

    for (VertexInd neighbor : mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_)
        update_quadrics_around_vertex(neighbor);
}

void EdgeCollapse::QuadricEdgeCollapse::init_Qs()
{
    vertex_Qs_.resize(mesh_.get_vert_amt());
    size_t t_amt = mesh_.get_triang_amt();

    int degenerate_count = 0;

    for (TriangleInd t_ind = 0; t_ind < t_amt; t_ind++) 
    {
        const auto& t = mesh_.get_triangles()[t_ind];

        Eigen::Matrix4f K = get_K(t);

        if (has_nan(K) || is_degenerate(K)) 
        {
            degenerate_count++;
            continue;
        }

        vertex_Qs_[t[0]] = vertex_Qs_[t[0]] + K;
        vertex_Qs_[t[1]] = vertex_Qs_[t[1]] + K;
        vertex_Qs_[t[2]] = vertex_Qs_[t[2]] + K;
    }

}

Eigen::Matrix4f EdgeCollapse::QuadricEdgeCollapse::get_K(const Triangle& t) const 
{
    const auto& vertices = mesh_.get_vertices();
    Eigen::Vector3f v0 = toEigen(vertices[t[0]]);
    Eigen::Vector3f v1 = toEigen(vertices[t[1]]);
    Eigen::Vector3f v2 = toEigen(vertices[t[2]]);
    
    Eigen::Vector3f normal = (v1 - v0).cross(v2 - v0).normalized();
    float d = -normal.dot(v0);
    
    Eigen::Vector4f p(normal.x(), normal.y(), normal.z(), d);
    return p * p.transpose();
}