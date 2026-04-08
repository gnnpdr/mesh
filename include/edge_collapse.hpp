#pragma once

#include "mesh.hpp"
#include <queue>
#include <memory>
#include <algorithm>

namespace EdgeCollapse
{
    using Mesh::EdgeInd;
    using Mesh::EdgeMesh;
    using Mesh::VertexInd;
    using Mesh::TriangleInd;

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
    VertexInd ERROR_VAL = 999;
    size_t MAX_TRIANGLES_AMT = 4;
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

public:
    explicit EdgeCollapse(Mesh::EdgeMesh& mesh) : mesh_(mesh)
    {
        build_heap();
    }

    EdgeCollapse(Mesh::EdgeMesh& mesh, size_t final_triangles_amt) : mesh_(mesh), final_triangles_amt_(final_triangles_amt)
    {
        build_heap();
    }

    EdgeCollapse(Mesh::EdgeMesh& mesh, float detail_level) : mesh_(mesh), detail_level_(detail_level), final_triangles_amt_(calc_final_triangles_amt())
    {
        build_heap();
    }

    EdgeCollapse(const EdgeCollapse&) = delete;             //чтобы ссылка не перепривязывалась
    EdgeCollapse& operator=(const EdgeCollapse&) = delete;
    
    EdgeCollapse(EdgeCollapse&&) = default;
    EdgeCollapse& operator=(EdgeCollapse&&) = default;

    EdgeMesh& simplify() 
    {   
        while (mesh_.get_triangles().size() > final_triangles_amt_ && !edge_heap_.empty()) 
        {
            EdgePtr best = get_best_edge();

            collapse_edge(best.edge_ind_);
            update_edges_in_heap(best.edge_ind_);
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

    void build_heap() 
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

    

    void collapse_edge(EdgeInd edge_ind) 
    {
        auto& vertices = mesh_.get_vertices();
        auto& triangles = mesh_.get_triangles();
        Mesh::Edge& edge_to_remove = mesh_.get_edges()[edge_ind];

        VertexInd vert_to_keep = edge_to_remove.v1_;
        VertexInd vert_to_remove = edge_to_remove.v2_;

        Vec3::Vec3f new_pos = calc_new_pos(vert_to_keep, vert_to_remove);
        mesh_.set_vert(new_pos, vert_to_keep);

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

            TriangleInd new_ind = new_triangles.size();  // ← индекс в НОВОМ векторе
            new_triangles.push_back(triangle);

            for (int i = 0; i < 3; i++)
                mesh_.get_vertex_data()[triangle[i]].incident_triangles_.push_back(new_ind);
        }

        mesh_.set_triangles(new_triangles);
    }

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
    }
};

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

public:
    SimpleEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
    }

    ~SimpleEdgeCollapse() = default;
};

class QuadricEdgeCollapse : public EdgeCollapse
{
protected:

    Vec3::Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override
    {

    }

    float calc_edge_cost(EdgeInd e) override
    {

    }

public:

    QuadricEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
    }

    ~QuadricEdgeCollapse() = default;
};

}