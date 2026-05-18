
#pragma once
#include "mesh/mesh.hpp"

namespace MyAlgorithms 
{

class MyCoolAlgorithm 
{
public:
    MyCoolAlgorithm(const Mesh::Mesh& mesh, float detail_level);
    Mesh::Mesh simplify();
    
private:
    const Mesh::Mesh& mesh_;
    float detail_level_;
};

}