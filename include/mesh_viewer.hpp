#pragma once
#include "vertex_cluster.hpp"
#include "metrics.hpp"
#include "edge_collapse.hpp"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include <polyscope/curve_network.h>

namespace MeshViewer
{

enum AlgorithmType
{
    VERTEX_CLUSTER,
    EDGE_COLLAPSE,
    QUADRIC
};

class MeshViewer
{

    Mesh::Mesh orig_mesh_;
    Mesh::Mesh current_mesh_;
    AlgorithmType algo_ = VERTEX_CLUSTER;
    float detail_level_ = 0;

    std::string algo_name_ = "Vertex cluster";
    float current_hausdorff_ = 0.0f;
    float current_rms_ = 0.0f;

    polyscope::SurfaceMesh* ps_mesh_ = nullptr;

    float min_detail_ = 0;
    float max_detail_ = 1.0f;
    int min_faces_ = 3;

    void update_detail_range() 
    {
        int orig_faces = orig_mesh_.get_triang_amt();
        
        min_detail_ = 0.0f;

        int target_faces = 3;
        max_detail_ = 1.0f - (float)target_faces / orig_faces - 0.5f;

        if (detail_level_ > max_detail_)
            detail_level_ = max_detail_;

        if (detail_level_ < min_detail_)
            detail_level_ = min_detail_;
    }

public:

    MeshViewer(const Mesh::Mesh& orig_mesh) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh) 
    {
        update_detail_range();
        update_metrics();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, AlgorithmType algo) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), algo_(algo) 
    {
        update_detail_range();
        update_metrics();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, float detail_level) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), detail_level_(detail_level) 
    {
        update_detail_range();
        update_metrics();
    }
    MeshViewer(const Mesh::Mesh& orig_mesh, AlgorithmType algo, float detail_level) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), algo_(algo), detail_level_(detail_level) 
    {
        update_detail_range();
        update_metrics();
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
                algo_name_ = "Vertex cluster";
                VertexCluster::VertexCluster simplifier(orig_mesh_, detail_level_);
                result = simplifier.simplify();
                break;
            }
            case EDGE_COLLAPSE: 
            {
                algo_name_ = "Edge collapse";
                Mesh::EdgeMesh edge_orig_mesh(orig_mesh_);
                EdgeCollapse::SimpleEdgeCollapse simplifier(edge_orig_mesh, detail_level_);
                result = simplifier.simplify();
                break;
            }
            case QUADRIC: 
            {
                algo_name_ = "Quadric";
                Mesh::EdgeMesh edge_orig_mesh(orig_mesh_);
                EdgeCollapse::QuadricEdgeCollapse simplifier(edge_orig_mesh, detail_level_);
                result = simplifier.simplify();
                break;
            }
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
        static bool warned_about_edge_collapse = false;
        static bool was_dragging = false;
        static bool last_is_dragging = false; 

        if (ImGui::Button("Vertex Cluster")) 
        {
            algo_ = VERTEX_CLUSTER;
            simplify_and_update();
            update_metrics();
        }
        ImGui::SameLine();
        if (ImGui::Button("Edge Collapse")) 
        {
            algo_ = EDGE_COLLAPSE;
            simplify_and_update();
            update_metrics();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quadric")) 
        {
            algo_ = QUADRIC;
            simplify_and_update();
            update_metrics();
        }


        if (algo_ == EDGE_COLLAPSE && !warned_about_edge_collapse) 
        {
            ImGui::OpenPopup("Edge Collapse Warning");
            warned_about_edge_collapse = true;
        }

        if (ImGui::BeginPopupModal("Edge Collapse Warning")) 
        {
            ImGui::Text("Edge Collapse is significantly slower than Vertex Cluster.");
            ImGui::Text("It may take several seconds for large models.");
            ImGui::Text("The simplification will run when you release the slider.");

            if (ImGui::Button("OK, honey"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        bool changed = ImGui::SliderFloat("Detail Level", &detail_level_, min_detail_, max_detail_, "%.3f");
        bool is_dragging = ImGui::IsItemActive();

        if (algo_ == VERTEX_CLUSTER || algo_ == QUADRIC) 
        {
            if (changed) 
            {
                simplify_and_update();
                update_metrics();
            }
        } 
        else 
        { 
            if (last_is_dragging && !is_dragging) 
            {
                simplify_and_update();
                update_metrics();
            }

            if (is_dragging)
                ImGui::TextColored(ImVec4(1,1,0,1), "Release slider to apply edge collapse and wait (please, sir)");

            last_is_dragging = is_dragging;
        }

        metrics_callback();
    }

    void update_metrics() 
    {
        Metrics::Metrics metrics(orig_mesh_, current_mesh_);
        current_hausdorff_ = metrics.get_hausdorff() * 100;
        current_rms_ = metrics.get_rms() * 100;
    }

    void metrics_callback() 
    {

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, ImGui::GetIO().DisplaySize.y - 280), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 260), ImGuiCond_Always);
        
        ImGui::Begin("Mesh Simplification Metrics", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Algorithm: %s", algo_name_.c_str());

        ImGui::Separator();

        ImGui::Text("Quality Metrics:");
        ImGui::Indent();

        ImGui::Text("Hausdorff distance:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.6f", current_hausdorff_);

        ImGui::Text("RMS error:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.6f", current_rms_);

        ImGui::Separator();

        ImGui::End();
    }

    // ImVec4 get_color_for_metrics(float value, float maxGood = 0.01f) 
    // {
    //     if (value < maxGood) return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    //     if (value < maxGood * 3) return ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    //     return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    // }

};
}