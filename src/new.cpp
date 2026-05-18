#include "algorithms/new.hpp"

namespace MyAlgorithms 
{

MyCoolAlgorithm::MyCoolAlgorithm(const Mesh::Mesh& mesh, float detail_level) : mesh_(mesh), detail_level_(detail_level){}

Mesh::Mesh MyCoolAlgorithm::simplify() 
{
    Mesh::Mesh result = mesh_;
    
    return result;
}

}