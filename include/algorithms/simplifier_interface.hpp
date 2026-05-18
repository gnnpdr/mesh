#pragma once

#include "mesh/mesh.hpp"

namespace MeshSimplify 
{
class ISimplifier 
{
public:
    virtual ~ISimplifier() = default;
    
    virtual std::string get_name() const = 0;

    virtual Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) = 0;
};

}