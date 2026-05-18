#pragma once

#include "mesh/mesh.hpp"
#include <queue>
#include <memory>
#include <algorithm>
#include <iostream>

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

    virtual Vec3f calc_new_pos(VertexInd v1, VertexInd v2) = 0;

    virtual float calc_edge_cost(EdgeInd e) = 0;

    void build_heap();

    //здесь нам нужно убрать вырожденные треугольники. если мы просто пройдемся по массиву с проверкой и сделаем новый, то возникнет проблема с тем, что
    //индексы в инцедентных треугольниках старые, нужно просто почистить все, что было и построить снова((
    void remove_degenerate_triangles();

public:
    explicit EdgeCollapse(Mesh::EdgeMesh& mesh) : mesh_(mesh) {}

    EdgeCollapse(Mesh::EdgeMesh& mesh, size_t final_triangles_amt) : mesh_(mesh), final_triangles_amt_(final_triangles_amt) {}

    EdgeCollapse(Mesh::EdgeMesh& mesh, float detail_level) : mesh_(mesh), detail_level_(detail_level), final_triangles_amt_(calc_final_triangles_amt()) {}

    EdgeCollapse(const EdgeCollapse&) = delete;  
    EdgeCollapse& operator=(const EdgeCollapse&) = delete;
    
    EdgeCollapse(EdgeCollapse&&) = default;
    EdgeCollapse& operator=(EdgeCollapse&&) = default;

    EdgeMesh& simplify();

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

    EdgePtr get_best_edge();

    virtual void collapse_edge(EdgeInd edge_ind) = 0;    

    void update_edges_in_heap(EdgeInd collapsed_edge_ind);

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

    Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override
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

     void collapse_edge(EdgeInd edge_ind) override;

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
    std::vector<Eigen::Matrix4f> vertex_Qs_;

protected:

    Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override;

    //похоже, придется считать новую позицию дважды, потому что у нас функции виртуальные и нельзя менять их параметры и выходное значение
    float calc_edge_cost(EdgeInd e);

    void update_quadrics_around_vertex(VertexInd v) 
    {
        // Полностью пересчитываем квадрику с нуля
        vertex_Qs_[v] = Eigen::Matrix4f::Zero();
        
        for (TriangleInd t_idx : mesh_.get_vertex_data()[v].incident_triangles_) {
            const auto& t = mesh_.get_triangles()[t_idx];
            Eigen::Matrix4f K = get_K(t);

            if (!has_nan(K) && !is_degenerate(K)) {
                vertex_Qs_[v] += K;
            }
        }
    }

    void collapse_edge(EdgeInd edge_ind) override;

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
    
    float evaluate_error(const Eigen::Matrix4f& Q, const Vec3f& v) 
    {
        Eigen::Vector4f v4(v.x(), v.y(), v.z(), 1.0f);
        return v4.transpose() * Q * v4;
    }

    void init_Qs();

    Eigen::Matrix4f get_K(const Triangle& t) const;

};

}