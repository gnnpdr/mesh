/**
 * @file adapters.hpp
 * @brief Адаптеры для регистрации алгоритмов упрощения в плагинной системе
 * 
 * Этот файл содержит классы-адаптеры, которые оборачивают существующие алгоритмы упрощения в единый интерфейс ISimplifier.
 * Это позволяет динамически подключать алгоритмы без изменения основного кода.
 */

#pragma once

#include "simplifier_interface.hpp"
#include "algorithms/vertex_cluster.hpp"
#include "algorithms/edge_collapse.hpp"
#include "mesh/mesh.hpp"

namespace MeshSimplify 
{

class VertexClusterAdapter : public ISimplifier 
{
public:
    /**
     * @brief Возвращает название алгоритма
     * @return Строка "Edge Collapse"
     */
    std::string get_name() const override { return "Vertex Cluster"; }
    
    /**
     * @brief Выполняет упрощение сетки алгоритмом кластеризации вершин
     * @param mesh Исходная сетка для упрощения
     * @param detail_level Степень детализации (0.0 - максимальное упрощение, 1.0 - без изменений)
     * @return Упрощённая сетка
     * @throws std::invalid_argument Если detail_level не в диапазоне [0, 1] или mesh пуста
     * @throws std::runtime_error Если упрощение не удалось
     */
    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        // Проверка входных параметров
        if (detail_level < 0.0f || detail_level > 1.0f)
            throw std::invalid_argument("VertexClusterAdapter::simplify: detail_level must be in [0, 1], got " + std::to_string(detail_level));
        
        if (mesh.is_empty())
            throw std::invalid_argument("VertexClusterAdapter::simplify: input mesh is empty");
        
        try {
            VertexCluster::VertexCluster simplifier(mesh, detail_level);
            Mesh::Mesh result = simplifier.simplify();
            
            if (result.is_empty())
                throw std::runtime_error("VertexClusterAdapter::simplify: simplification produced empty mesh");
            
            return result;
        } 
        catch (const std::exception& e)
        {
            throw std::runtime_error("VertexClusterAdapter::simplify failed: " + std::string(e.what()));
        }
    }
};


class EdgeCollapseAdapter : public ISimplifier 
{
public:
   
    std::string get_name() const override { return "Edge Collapse"; }
    
    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        if (detail_level < 0.0f || detail_level > 1.0f)
            throw std::invalid_argument("EdgeCollapseAdapter::simplify: detail_level must be in [0, 1], got " + std::to_string(detail_level));
        
        if (mesh.is_empty())
            throw std::invalid_argument("EdgeCollapseAdapter::simplify: input mesh is empty");
        
        try 
        {
            float target = 1.0f - detail_level;
            Mesh::EdgeMesh edge_mesh(mesh);
            EdgeCollapse::SimpleEdgeCollapse simplifier(edge_mesh, target);
            Mesh::Mesh result = simplifier.simplify();
            
            if (result.is_empty())
                throw std::runtime_error("EdgeCollapseAdapter::simplify: simplification produced empty mesh");
            
            return result;
        } 
        catch (const std::exception& e)
        {
            throw std::runtime_error("EdgeCollapseAdapter::simplify failed: " + std::string(e.what()));
        }
    }
};

class QuadricAdapter : public ISimplifier 
{
public:
    std::string get_name() const override { return "Quadric"; }

    Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
    {
        if (detail_level < 0.0f || detail_level > 1.0f)
            throw std::invalid_argument("QuadricAdapter::simplify: detail_level must be in [0, 1], got " + std::to_string(detail_level));
        
        if (mesh.is_empty()) 
            throw std::invalid_argument("QuadricAdapter::simplify: input mesh is empty");
        
        try 
        {
            float target = 1.0f - detail_level;
            Mesh::EdgeMesh edge_mesh(mesh);
            EdgeCollapse::QuadricEdgeCollapse simplifier(edge_mesh, target);
            Mesh::Mesh result = simplifier.simplify();
            
            if (result.is_empty())
                throw std::runtime_error("QuadricAdapter::simplify: simplification produced empty mesh");
            
            return result;
        } 
        catch (const std::exception& e) 
        {
            throw std::runtime_error("QuadricAdapter::simplify failed: " + std::string(e.what()));
        }
    }
};

// Пример добавления нового алгоритма:
// class MyCoolAlgorithmAdapter : public ISimplifier {
// public:
//     std::string get_name() const override { return "My Cool Algorithm"; }
//
//     Mesh::Mesh simplify(const Mesh::Mesh& mesh, float detail_level) override 
//     {
//         if (detail_level < 0.0f || detail_level > 1.0f)
//             throw std::invalid_argument("detail_level out of range");
//         
//         MyAlgorithms::MyCoolAlgorithm simplifier(mesh, detail_level);
//         return simplifier.simplify();
//     }
// };

}