#pragma once
#include "vertex_cluster.hpp"
//#include "edhe_collapse.hpp"
//#include "quadric.hpp"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

namespace MeshViewer
{

enum AlgorithmType
{
    VERTEX_CLUSTER//,
    //EDGE_COLLAPSE,
    //QUADRIC
};

class MeshViewer
{

    Mesh::Mesh orig_mesh_;
    Mesh::Mesh current_mesh_;
    AlgorithmType algo_ = VERTEX_CLUSTER;
    float detail_level_ = 0;

    polyscope::SurfaceMesh* ps_mesh_ = nullptr;

    float min_detail_ = 0;
    float max_detail_ = 1.0f;
    int min_faces_ = 3;

    void updateDetailRange() 
    {
        int orig_faces = orig_mesh_.get_triang_amt();
        
        min_detail_ = 0.0f;

        int target_faces = 3;
        max_detail_ = 1.0f - (float)target_faces / orig_faces - 0.5f;

        if (detail_level_ > max_detail_) {
            detail_level_ = max_detail_;
        }
        if (detail_level_ < min_detail_) {
            detail_level_ = min_detail_;
        }
    }

public:

    MeshViewer(const Mesh::Mesh& orig_mesh) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh) 
    {
        updateDetailRange();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, AlgorithmType algo) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), algo_(algo) 
    {
        updateDetailRange();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, float detail_level) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), detail_level_(detail_level) 
    {
        updateDetailRange();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, AlgorithmType algo, float detail_level) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), algo_(algo), detail_level_(detail_level) 
    {
        updateDetailRange();
    }

    void show_mesh() 
    {
        polyscope::init();
        
        ps_mesh_ = polyscope::registerSurfaceMesh("Mesh", orig_mesh_.get_vertices(), orig_mesh_.get_triangles()
        );
        
        polyscope::state::userCallback = [this]() {
            this->draw_ui();
        };

        polyscope::show();
    }

private:

    void simplify_and_update() 
    {
        Mesh::Mesh result;
        
        switch(algo_) 
        {
            case VERTEX_CLUSTER: 
            {
                VertexCluster::VertexCluster simplifier(orig_mesh_, detail_level_);
                result = simplifier.simplify();
                break;
            }
            //case EDGE_COLLAPSE: 
            //{
            //    EdgeCollapse simplifier;
            //    result = simplifier.simplify(original_mesh_, detail_level_);
            //    break;
            //}
            //case QUADRIC: 
            //{
            //    QuadricSimplifier simplifier;
            //    result = simplifier.simplify(original_mesh_, detail_level_);
            //    break;
            //}
            default:
                result = orig_mesh_;
        }
        
        current_mesh_ = result;

        if (ps_mesh_) 
        {
            std::string name = ps_mesh_->name;
            
            polyscope::removeSurfaceMesh(name);
            
            ps_mesh_ = polyscope::registerSurfaceMesh(name, current_mesh_.get_vertices(), current_mesh_.get_triangles());
        }
    }

    void draw_ui() 
    {
        if (ImGui::Button("Vertex Cluster")) 
        {
            algo_ = VERTEX_CLUSTER;
            simplify_and_update();
        }
        //ImGui::SameLine();
        //if (ImGui::Button("Edge Collapse")) {
        //    current_algo_ = EDGE_COLLAPSE;
        //    simplify_and_update();
        //}
        //ImGui::SameLine();
        //if (ImGui::Button("Quadric")) {
        //    current_algo_ = QUADRIC;
        //    simplify_and_update();
        //}

        //ImGui::Text("Original faces: %d", original_mesh_.faceCount());
        //ImGui::Text("Min possible: %d faces (%.2f%%)", 
        //    min_faces_, min_detail_ * 100);
        
        //bool changed = ImGui::SliderFloat("Detail Level", &detail_level_, 0.01f, 1.0f, "%.2f");
        bool changed = ImGui::SliderFloat("Detail Level", &detail_level_, min_detail_, max_detail_, "%.3f");
        if (changed)
            simplify_and_update();
    }
};
}