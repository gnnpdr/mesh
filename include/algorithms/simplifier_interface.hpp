/**
 * @file simplifier_interface.hpp
 * @brief Интерфейс для плагинной системы алгоритмов упрощения
 * 
 * Абстрактный базовый класс ISimplifier служит единым интерфейсом для всех алгоритмов упрощения сеток. Это позволяет динамически подключать новые алгоритмы без изменения основного кода программы
 * 
 */
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