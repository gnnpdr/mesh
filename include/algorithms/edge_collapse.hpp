#pragma once

#include "mesh/mesh.hpp"
#include "lib/matrix.hpp"
#include <queue>
#include <memory>
#include <algorithm>

namespace EdgeCollapse
{
    using Mesh::EdgeInd;
    using Mesh::EdgeMesh;
    using Mesh::VertexInd;
    using Mesh::TriangleInd;
    using Mesh::Triangle;

struct EdgePtr 
{
    EdgeInd edge_ind_;  
    float cost_;

    bool operator<(const EdgePtr& other) const 
    {
        return cost_ > other.cost_; 
    }
};

namespace Detail
{
    static VertexInd ERROR_VAL = -1;
    static size_t MAX_TRIANGLES_AMT = 4;
}

class EdgeCollapse
{
protected:
    EdgeMesh& mesh_;        //да, только edgemesh. создавать, передавать mesh и создавать его прям тут накладно. пусть это делают вне.

    std::priority_queue<EdgePtr> edge_heap_;

    float detail_level_ = 0.5f;
    size_t final_triangles_amt_ = 400;

    virtual Vec3::Vec3f calc_new_pos(VertexInd v1, VertexInd v2) = 0;

    virtual float calc_edge_cost(EdgeInd e) = 0;

    void build_heap() 
    {
        auto& edges = mesh_.get_edges();
        size_t edges_amt = edges.size();
        for (EdgeInd e = 0; e < edges_amt; e++) 
        {
            if (edges[e].is_active_) 
            {
                float cost = calc_edge_cost(e);
                //Vec3::Vec3f pos = calc_new_pos(edges[e].v1_, edges[e].v2_);
                edge_heap_.push({e, cost});
            }
        }
    }

    //здесь нам нужно убрать вырожденные треугольники. если мы просто пройдемся по массиву с проверкой и сделаем новый, то возникнет проблема с тем, что
    //индексы в инцедентных треугольниках старые, нужно просто почистить все, что было и построить снова((
    void remove_degenerate_triangles() 
    {
        for (auto& vd : mesh_.get_vertex_data())
            vd.incident_triangles_.clear();

        std::vector<std::array<VertexInd, 3>> new_triangles;
        auto& triangles = mesh_.get_triangles();

        for (const auto& triangle : triangles)  // проходим по старым треугольникам
        {
            if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[2] == triangle[0])
                continue;

            TriangleInd new_ind = new_triangles.size();
            new_triangles.push_back(triangle);

            for (int i = 0; i < 3; i++)
                mesh_.get_vertex_data()[triangle[i]].incident_triangles_.push_back(new_ind);
        }

        mesh_.set_triangles(new_triangles);
    }

public:
    explicit EdgeCollapse(Mesh::EdgeMesh& mesh) : mesh_(mesh)
    {
        //build_heap();
    }

    EdgeCollapse(Mesh::EdgeMesh& mesh, size_t final_triangles_amt) : mesh_(mesh), final_triangles_amt_(final_triangles_amt)
    {
        //build_heap();
    }

    EdgeCollapse(Mesh::EdgeMesh& mesh, float detail_level) : mesh_(mesh), detail_level_(detail_level), final_triangles_amt_(calc_final_triangles_amt())
    {
        //build_heap();
    }

    EdgeCollapse(const EdgeCollapse&) = delete;             //чтобы ссылка не перепривязывалась
    EdgeCollapse& operator=(const EdgeCollapse&) = delete;
    
    EdgeCollapse(EdgeCollapse&&) = default;
    EdgeCollapse& operator=(EdgeCollapse&&) = default;

    EdgeMesh& simplify() 
    {   
        size_t last_count = mesh_.get_triangles().size();
        int iteration = 0;
        while (mesh_.get_triangles().size() > final_triangles_amt_ && !edge_heap_.empty()) 
        {
            EdgePtr best = get_best_edge();
            if (best.edge_ind_ == -1) break;

            collapse_edge(best.edge_ind_);
            update_edges_in_heap(best.edge_ind_);
            iteration++;

            if (iteration % 100 == 0) {
            size_t current = mesh_.get_triangles().size();
            std::cout << "Iter " << iteration << ": triangles " << current 
                      << " (target " << final_triangles_amt_ << ")" << std::endl;
            
            if (current >= last_count) {
                std::cout << "WARNING: Triangle count not decreasing!" << std::endl;
                break;
            }
            last_count = current;
        }
        }

        return mesh_;
    }

    virtual ~EdgeCollapse() = default;

private:

    size_t calc_final_triangles_amt()
    {
        size_t orig_triangles_amt = mesh_.get_triang_amt();
        size_t final_triangles_amt = (size_t)(orig_triangles_amt * detail_level_);

        if (final_triangles_amt < Detail::MAX_TRIANGLES_AMT)
            final_triangles_amt = Detail::MAX_TRIANGLES_AMT;
        if (final_triangles_amt > orig_triangles_amt)
            final_triangles_amt = orig_triangles_amt;

        return final_triangles_amt;
    }

    EdgePtr get_best_edge() 
    {
        while (!edge_heap_.empty()) 
        {
            EdgePtr top = edge_heap_.top();
            
            const Mesh::Edge& edge = mesh_.get_edges()[top.edge_ind_];
            
            if (!edge.is_active_) 
            {
                edge_heap_.pop();
                continue;
            }
            
            float current_cost = calc_edge_cost(top.edge_ind_);
            if (std::abs(current_cost - top.cost_) > 1e-6) 
            {
                edge_heap_.pop();
                continue;
            }
            
            return top;
        }
        return {Detail::ERROR_VAL, 0.0f};
    }

    virtual void collapse_edge(EdgeInd edge_ind) = 0;    

    void update_edges_in_heap(EdgeInd collapsed_edge_ind) 
    {
        auto& edges = mesh_.get_edges();
        const Mesh::Edge& collapsed_edge = edges[collapsed_edge_ind];
        VertexInd v = collapsed_edge.v1_;

        const auto& neighbors = mesh_.get_vertex_data()[v].neighbor_vertices_;

        for (VertexInd neighbor : neighbors) 
        {
            EdgeInd e_ind = mesh_.find_edge(v, neighbor);
            if (e_ind == -1) continue;

            float new_cost = calc_edge_cost(e_ind);

            // Просто добавляем новую запись в кучу
            // Старая останется, но будет проигнорирована при извлечении
            edge_heap_.push({e_ind, new_cost});
        }

        if (edge_heap_.size() > 1000000) {
            std::cout << "Heap too large, rebuilding..." << std::endl;
            rebuild_heap();
        }
    }

    void rebuild_heap() {
        std::priority_queue<EdgePtr> new_heap;
        for (EdgeInd e = 0; e < mesh_.get_edges().size(); e++) {
            if (mesh_.get_edges()[e].is_active_) {
                float cost = calc_edge_cost(e);
                new_heap.push({e, cost});
            }
        }
        edge_heap_ = std::move(new_heap);
    }

};



//----------------------------------------------------------------------------------------------------------------------------

class SimpleEdgeCollapse : public EdgeCollapse
{
protected:

    Vec3::Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override
    {
        auto& vertices = mesh_.get_vertices();
        return (vertices[v1] + vertices[v2]) * 0.5f;
    }

    float calc_edge_cost(EdgeInd e) override
    {
        auto& edges = mesh_.get_edges();
        auto& vertices = mesh_.get_vertices();

        auto& v1 = vertices[edges[e].v1_];
        auto& v2 = vertices[edges[e].v2_];

        return Vec3::distance(v1, v2);
    }

     void collapse_edge(EdgeInd edge_ind) override
    {
        auto& vertices = mesh_.get_vertices();
        auto& triangles = mesh_.get_triangles();
        Mesh::Edge& edge_to_remove = mesh_.get_edges()[edge_ind];

        VertexInd vert_to_keep = edge_to_remove.v1_;
        VertexInd vert_to_remove = edge_to_remove.v2_;

        Vec3::Vec3f new_pos = calc_new_pos(vert_to_keep, vert_to_remove);
        mesh_.set_vert(new_pos, vert_to_keep);

        auto& triangles_to_update = mesh_.get_vertex_data()[vert_to_remove].incident_triangles_;  
        for (TriangleInd triangle_ind : triangles_to_update) 
        {
            auto& triangle = triangles[triangle_ind];
            for (int i = 0; i < 3; i++) 
            {
                if (triangle[i] == vert_to_remove)
                {
                    mesh_.set_triangle_vert(vert_to_keep, triangle_ind, i);
                }
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

public:
    SimpleEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
        build_heap();
    }

    ~SimpleEdgeCollapse() = default;
};

//----------------------------------------------------------------------------------------------------------------------------------

class QuadricEdgeCollapse : public EdgeCollapse
{
    std::vector<Matrix::Quadric> vertex_Qs_;

protected:

    Vec3::Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override
    {
        Matrix::Quadric Q = vertex_Qs_[v1] + vertex_Qs_[v2];

        if (Q.has_nan() || Q.is_degenerate()) {
            std::cout << "NaN in Quadric sum for edge (" << v1 << "," << v2 << ")" << std::endl;
            return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;
        }

        Matrix::Matrix<3,3> A = Q.get_sub_A();      //конечно, надо бы вставить проверку
        Matrix::Vector<3> B = Q.get_B();

        // Проверка на вырожденность A
        float det = A.determinant();
        
        if (std::abs(det) < 1e-8f) 
            return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;
    
        Gauss::Solver solver(A, B);
        Matrix::Vector<3> optimal_pos = solver.solve();

        // Проверка результата на NaN
        if (std::isnan(optimal_pos(0,0)) || std::isnan(optimal_pos(1,0)) || std::isnan(optimal_pos(2,0)))
            return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;

        Vec3::Vec3f result(optimal_pos(0,0), optimal_pos(1,0), optimal_pos(2,0));

        float max_dist = 1000.0f;  // разумный порог
        if (result.length() > max_dist) {
            std::cout << "WARNING: Optimal position too far, using midpoint" << std::endl;
            return (mesh_.get_vertices()[v1] + mesh_.get_vertices()[v2]) * 0.5f;
        }

        return result;
    }

    //похоже, придется считать новую позицию дважды, потому что у нас функции виртуальные и нельзя менять их параметры и выходное значение
    float calc_edge_cost(EdgeInd e) 
    { 
        const auto& edge = mesh_.get_edges()[e];
        auto v1 = edge.v1_;
        auto v2 = edge.v2_;

        Matrix::Quadric sum_Q = vertex_Qs_[v1] + vertex_Qs_[v2];

        if (sum_Q.has_nan() || sum_Q.is_degenerate()) {
            // Fallback на длину ребра
            return Vec3::distance(mesh_.get_vertices()[v1], mesh_.get_vertices()[v2]);
        }

        Vec3::Vec3f optimal_pos = calc_new_pos(v1, v2);
        float error = evaluate_error(sum_Q, optimal_pos);

        if (std::isnan(error) || std::isinf(error)) {
            return Vec3::distance(mesh_.get_vertices()[v1], mesh_.get_vertices()[v2]);
        }

        return error;
    }

    void update_quadrics_around_vertex(VertexInd v) 
    {
        // Полностью пересчитываем квадрику с нуля
        vertex_Qs_[v] = Matrix::Quadric();
        
        for (TriangleInd t_idx : mesh_.get_vertex_data()[v].incident_triangles_) {
            const auto& t = mesh_.get_triangles()[t_idx];
            Matrix::Quadric K = get_K(t);

            if (!K.has_nan() && !K.is_degenerate()) {
                vertex_Qs_[v] = vertex_Qs_[v] + K;
            }
        }
    }

     void collapse_edge(EdgeInd edge_ind) override
    {
        auto& vertices = mesh_.get_vertices();
        auto& triangles = mesh_.get_triangles();
        Mesh::Edge& edge_to_remove = mesh_.get_edges()[edge_ind];

        VertexInd vert_to_keep = edge_to_remove.v1_;
        VertexInd vert_to_remove = edge_to_remove.v2_;

        Vec3::Vec3f new_pos = calc_new_pos(vert_to_keep, vert_to_remove);
        mesh_.set_vert(new_pos, vert_to_keep);

        vertex_Qs_[vert_to_keep] = vertex_Qs_[vert_to_keep] + vertex_Qs_[vert_to_remove];

        //переносим треугольники с удаленной вершины на оставшуюся
        auto& triangles_to_update = mesh_.get_vertex_data()[vert_to_remove].incident_triangles_;  
        for (TriangleInd triangle_ind : triangles_to_update) 
        {
            auto& triangle = triangles[triangle_ind];
            for (int i = 0; i < 3; i++) 
            {
                if (triangle[i] == vert_to_remove)
                {
                    mesh_.set_triangle_vert(vert_to_keep, triangle_ind, i);
                }
            }
        }

        std::set<VertexInd> vertices_to_update;
        vertices_to_update.insert(vert_to_keep);
        for (VertexInd neighbor : mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_) {
            vertices_to_update.insert(neighbor);
        }

        for (VertexInd v : vertices_to_update) 
            update_quadrics_around_vertex(v);
    
        //обновляем соседей
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
        keep_neighbors.erase(vert_to_keep);     //на всякий случай

        //деактивация
        mesh_.get_vertex_data()[vert_to_remove].is_active_ = false;
        edge_to_remove.is_active_ = false;

        remove_degenerate_triangles();

        for (VertexInd neighbor : mesh_.get_vertex_data()[vert_to_keep].neighbor_vertices_) {
            update_quadrics_around_vertex(neighbor);
        }
    }

public:

    QuadricEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
        std::cout << "QuadricEdgeCollapse constructor start\n";
        init_Qs();
        std::cout << "init_Qs done, vertex_Qs_.size() = " << vertex_Qs_.size() << "\n";
        build_heap();
        std::cout << "build_heap done\n";
    }

    ~QuadricEdgeCollapse() = default;

private:

    float evaluate_error(const Matrix::Quadric& Q, const Vec3::Vec3f& v) 
    {
        // v^T * Q * v, где v = [x, y, z, 1]
        float x = v.x(), y = v.y(), z = v.z();

        return Q(0,0) * x * x + 2 * Q(0,1) * x * y + 2 * Q(0,2) * x * z + 2 * Q(0,3) * x
             + Q(1,1) * y * y + 2 * Q(1,2) * y * z + 2 * Q(1,3) * y 
             + Q(2,2) * z * z + 2 * Q(2,3) * z 
             + Q(3,3);
    }

    void init_Qs()
    {
        vertex_Qs_.resize(mesh_.get_vert_amt());
        size_t t_amt = mesh_.get_triang_amt();

        int degenerate_count = 0;

        for (TriangleInd t_ind = 0; t_ind < t_amt; t_ind++) 
        {
            const auto& t = mesh_.get_triangles()[t_ind];

            Matrix::Quadric K = get_K(t);

            if (K.has_nan() || K.is_degenerate()) {
                degenerate_count++;
                continue;
            }

            vertex_Qs_[t[0]] = vertex_Qs_[t[0]] + K;
            vertex_Qs_[t[1]] = vertex_Qs_[t[1]] + K;
            vertex_Qs_[t[2]] = vertex_Qs_[t[2]] + K;
        }

        std::cout << "Initialized " << vertex_Qs_.size() << " quadrics, "
                  << degenerate_count << " degenerate triangles skipped" << std::endl;
    }

    Matrix::Quadric get_K(const Triangle& t) const {
        auto& vertices = mesh_.get_vertices();
        Vec3::Vec3f dot1 = vertices[t[0]];
        Vec3::Vec3f dot2 = vertices[t[1]];
        Vec3::Vec3f dot3 = vertices[t[2]];
        
        Vec3::Vec3f edge1 = dot2 - dot1;
        Vec3::Vec3f edge2 = dot3 - dot1;
        Vec3::Vec3f normal = edge1.cross(edge2);
        
        float area2 = normal.length(); 
        if (area2 < 1e-12f)
        {  
            std::cout << "Degenerate triangle detected: (" << t[0] << "," << t[1] << "," << t[2] << ")"
                      << " area2=" << area2 << std::endl;
            return Matrix::Quadric(); 
        }

        normal = normal/area2;

        float d = -normal.dot(dot1);

        Matrix::Vector<4> plane_vec;
        plane_vec(0,0) = normal.x();
        plane_vec(1,0) = normal.y();
        plane_vec(2,0) = normal.z();
        plane_vec(3,0) = d;

        auto plane_vec_t = plane_vec.transpose();
        return plane_vec * plane_vec_t;
    }

};

}