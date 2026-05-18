#pragma once

#include "simplifier_interface.hpp"
#include "algorithms/vertex_cluster.hpp"
#include "algorithms/edge_collapse.hpp"
#include "mesh/mesh.hpp" 
#include "algorithms/new.hpp"

namespace MeshSimplify 
{

class VertexClusterAdapter : public ISimplifier 
{
public:
    std::string get_name() const override { return "Vertex Cluster"; }
    
    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        VertexCluster::VertexCluster simplifier(mesh, detail_level);
        return simplifier.simplify();
    }
};

class EdgeCollapseAdapter : public ISimplifier 
{
public:
    std::string get_name() const override { return "Edge Collapse";}
    
    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        float target = 1.0f - detail_level;
        Mesh::EdgeMesh edge_mesh(mesh);
        EdgeCollapse::SimpleEdgeCollapse simplifier(edge_mesh, target);
        return simplifier.simplify();
    }
};

class QuadricAdapter : public ISimplifier 
{
public:
    std::string get_name() const override { return "Quadric";}
    
    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        float target = 1.0f - detail_level;
        Mesh::EdgeMesh edge_mesh(mesh);
        EdgeCollapse::QuadricEdgeCollapse simplifier(edge_mesh, target);
        return simplifier.simplify();
    }
};

// class MyCoolAlgorithmAdapter : public ISimplifier {
// public:
//     std::string get_name() const override { 
//         return "My Cool Algorithm"; 
//     }
    
//     Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override {
//         MyAlgorithms::MyCoolAlgorithm simplifier(mesh, detail_level);
//         return simplifier.simplify();
//     }
// };

}