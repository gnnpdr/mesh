/**
 * @file vertex_cluster.hpp
 * @brief Алгоритм упрощения сетки методом кластеризации вершин (распределения по ячейкам)
 * 
 * Пространство модели разбивается на регулярную сетку ячеек
 * Все вершины в одной ячейке объединяются в одну
 * Треугольники перестраиваются с использованием новых вершин
 * Вырожденные треугольники удаляются
 * 
 * Очень быстрый
 */

#pragma once

#include "mesh/mesh.hpp"
#include <map>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace VertexCluster
{
/**
 * @brief Позиция ячейки в трёхмерной сетке
 */
struct CellPos
{
    int x_, y_, z_;
    CellPos(int x, int y, int z) : x_(x), y_(y), z_(z) {}

    bool operator<(const CellPos& other) const 
    {
        if (x_ != other.x_) 
            return x_ < other.x_;
        if (y_ != other.y_) 
            return y_ < other.y_;
        return z_ < other.z_;
    }
};

/**
 * @brief Данные для каждой ячейки сетки
 * 
 * Сумма координат всех вершин в ячейке и их количество
 */
class CellData
{
    Vec3f sum_;        
    size_t verteces_amt_; 

public:
    CellData() : sum_(0.0f, 0.0f, 0.0f), verteces_amt_(0) {}

    void add(const Vec3f& v)
    {
        sum_ = sum_ + v;
        verteces_amt_++;
    }

    Vec3f calc_average_v()
    {
        if (verteces_amt_ == 0)
            return Vec3f(0.0f, 0.0f, 0.0f);

        return Vec3f(sum_.x() / verteces_amt_, 
                     sum_.y() / verteces_amt_, 
                     sum_.z() / verteces_amt_);
    }
    
    bool empty() const { return verteces_amt_ == 0; }
    
    size_t size() const { return verteces_amt_; }
};

/**
 * @brief Класс для упрощения
 */
class VertexCluster
{
    Mesh::Mesh input_mesh_;  ///< Исходная сетка
    
    Vec3f min_bound_;        ///< Минимальные границы модели
    Vec3f max_bound_;        ///< Максимальные границы модели
    
    float cell_size_;        ///< Размер ячейки сетки
    float detail_level_;     ///< Степень детализации

    std::map<CellPos, CellData> cells_;           ///< Ячейки с данными
    std::map<CellPos, size_t> cell_to_new_ind_;   ///< Ячейка - индекс новой вершины

    std::vector<int> old_to_new_v_;               ///< Старые индексы на новые 
    std::vector<Vec3f> new_vertices_;             ///< Новые вершины 
    std::vector<std::array<int, 3>> new_faces_;   ///< Новые треугольники

public:

    VertexCluster(const Mesh::Mesh& mesh) : input_mesh_(mesh), detail_level_(0.02f)
    {
        if (mesh.is_empty())
            throw std::invalid_argument("VertexCluster: input mesh is empty");
        
        find_bounds(mesh);
        update_cell_size();
        
        size_t vertices_amt = input_mesh_.get_vertices().size();
        old_to_new_v_.resize(vertices_amt, -1);
    } 

    VertexCluster(const Mesh::Mesh& mesh, float detail_level) : input_mesh_(mesh), detail_level_(detail_level)
    {
        if (mesh.is_empty())
            throw std::invalid_argument("VertexCluster: input mesh is empty");
    
        if (detail_level < 0.0f || detail_level > 1.0f)
            throw std::invalid_argument("VertexCluster: detail_level must be in [0, 1], got " + std::to_string(detail_level));
        
        find_bounds(mesh);
        update_cell_size();
        
        size_t vertices_amt = input_mesh_.get_vertices().size();
        old_to_new_v_.resize(vertices_amt, -1);
    } 

    Mesh::Mesh simplify();

private:
    /**
     * @brief Обновляет размер ячейки на основе bounding box и detail_level
     */
    void update_cell_size()
    {
        float max_model_size = std::max({max_bound_.x() - min_bound_.x(), 
                                         max_bound_.y() - min_bound_.y(), 
                                         max_bound_.z() - min_bound_.z()});
        if (max_model_size < 1e-6f) 
            throw std::runtime_error("VertexCluster: model size is too small (degenerate mesh)");
        
        cell_size_ = max_model_size * detail_level_;
        if (cell_size_ < 1e-6f) 
            cell_size_ = 0.01f; 
    }

    /**
     * @brief Находит bounding box модели
     */
    void find_bounds(const Mesh::Mesh& input_mesh)
    {
        std::array<Vec3f, 2> bounding_box = input_mesh.get_bounding_box();
        min_bound_ = bounding_box[0];
        max_bound_ = bounding_box[1];
        
        if (min_bound_.x() == max_bound_.x() && 
            min_bound_.y() == max_bound_.y() && 
            min_bound_.z() == max_bound_.z()) 
            throw std::runtime_error("VertexCluster: degenerate bounding box (all vertices at same point)");
    }

    /**
     * @brief Определяет позицию ячейки для заданной вершины
     */
    CellPos get_cell_pos(const Vec3f& v) 
    {
        return CellPos(std::floor((v.x() - min_bound_.x()) / cell_size_),
                       std::floor((v.y() - min_bound_.y()) / cell_size_),
                       std::floor((v.z() - min_bound_.z()) / cell_size_));
    }

    /**
     * @brief Группирует вершины по ячейкам
     */
    void cluster_vertices();

    /**
     * @brief Вычисляет новые вершины
     */
    void calc_new_vertices();

    /**
     * @brief Создаёт маппинг старых индексов вершин на новые
     */
    void old_new_v_match();

    /**
     * @brief Перестраивает треугольники с использованием новых вершин
     */
    void triangulate();
};

}