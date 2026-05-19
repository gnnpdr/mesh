/**
 * @file metrics.hpp
 * @brief Метрики для независисмой оченки качества упрощения
 * 
 * Из метрик самые распространенные
 * Расстояния Хаусдорфа - максимальное расстояние между сетками
 * Среднеквадратичная ошибка (среднее квадратов расстояний между сетками)
 * 
 * Обе метрики нормализуются по диагонали ограничивающего прямоугольника.
 */

#pragma once

#include <limits>
#include <cmath>
#include <stdexcept>
#include "mesh/mesh.hpp"
#include <iostream>

namespace Metrics
{

namespace Detail
{
    static float INF = std::numeric_limits<float>::infinity();  
    static float EPSILON = 1e-6f;                           
}

/**
 * @brief Класс для вычисления метрик
 */
class Metrics
{
    float hausdorff_;          ///< Абсолютное расстояние Хаусдорфа
    float rms_;                ///< Абсолютная среднеквадратичная ошибка
    float norm_hausdorff_;     ///< Нормализованное расстояние Хаусдорфа (0-1)
    float norm_rms_;           ///< Нормализованная среднеквадратичная ошибка (0-1)
    Mesh::Mesh orig_mesh_;     ///< Исходная сетка
    Mesh::Mesh simple_mesh_;   ///< Упрощённая сетка

public:

    Metrics(const Mesh::Mesh& orig_mesh, const Mesh::Mesh& simple_mesh) : orig_mesh_(orig_mesh), simple_mesh_(simple_mesh), hausdorff_(0.0f), rms_(0.0f), norm_hausdorff_(0.0f), norm_rms_(0.0f)
    {
        if (orig_mesh_.is_empty())
            throw std::invalid_argument("Metrics: original mesh is empty");
    
        if (simple_mesh_.is_empty())
            throw std::invalid_argument("Metrics: simplified mesh is empty");
        
        try 
        {
            count_hausdorff();
            count_rms();
        } 
        catch (const std::exception& e) 
        {
            throw std::runtime_error("Metrics calculation failed: " + std::string(e.what()));
        }

        float diag = orig_mesh_.get_bounding_box_diag_size();
        if (diag < Detail::EPSILON)
            throw std::runtime_error("Metrics: bounding box diagonal is too small (degenerate mesh)");

        norm_hausdorff_ = hausdorff_ / diag;
        norm_rms_ = rms_ / diag;
    }

    void print() const
    {
        std::cout << "=== Mesh Simplification Metrics ===" << std::endl;
        std::cout << "Hausdorff distance: " << norm_hausdorff_ * 100 << "%" << std::endl;
        std::cout << "RMS error: " << norm_rms_ * 100 << "%" << std::endl;
        std::cout << "===================================" << std::endl;
    }

    float get_hausdorff_norm() const { return norm_hausdorff_ * 100; }
    
    float get_rms_norm() const { return norm_rms_ * 100; }
    
    float get_hausdorff_abs() const { return hausdorff_; }
    
    float get_rms_abs() const { return rms_; }

private:
    /**
     * @brief Вычисляет расстояние Хаусдорфа между сетками
     * @return Абсолютное расстояние Хаусдорфа
     * @throws std::runtime_error Если вычисление не удалось
     * 
     * Для каждой вершины упрощённой сетки находим ближайшую вершину в исходной сетке. Хаусдорф — это максимум из этих расстояний.
     */
    float count_hausdorff()
    {
        const auto& orig_vert = orig_mesh_.get_vertices();
        const auto& simple_vert = simple_mesh_.get_vertices();
        
        if (orig_vert.empty())
            throw std::runtime_error("count_hausdorff: original mesh has no vertices");
    
        if (simple_vert.empty())
            throw std::runtime_error("count_hausdorff: simplified mesh has no vertices");
        
        float max_dist = 0.0f;

        for (const auto& s_v : simple_vert) 
        {
            float min_dist = Detail::INF;
            for (const auto& o_v : orig_vert) 
            {
                float dist = Vec3::distance(s_v, o_v);
                if (dist < min_dist)
                    min_dist = dist;
            }
            
            // Проверка на корректность расстояния
            if (min_dist == Detail::INF)
                throw std::runtime_error("count_hausdorff: failed to find nearest vertex");
            
            if (min_dist > max_dist)
                max_dist = min_dist;
        }

        if (max_dist < 0.0f)
            throw std::runtime_error("count_hausdorff: invalid max distance");

        hausdorff_ = max_dist;
        return hausdorff_;
    }

    /**
     * @brief Вычисляет среднеквадратичную ошибку (RMS) между сетками
     * @return Абсолютная среднеквадратичная ошибка
     * @throws std::runtime_error Если вычисление не удалось
     * 
     * Для каждой вершины упрощённой сетки находим ближайшую вершину в исходной сетке, возводим расстояние в квадрат, суммируем, делим на количество вершин и извлекаем квадратный корень
     */
    float count_rms()
    {
        const auto& orig_vert = orig_mesh_.get_vertices();
        const auto& simple_vert = simple_mesh_.get_vertices();
        
        if (orig_vert.empty())
            throw std::runtime_error("count_rms: original mesh has no vertices");

        if (simple_vert.empty()) 
            throw std::runtime_error("count_rms: simplified mesh has no vertices");
        
        float sum_dist_sq = 0.0f;
        size_t s_v_amt = simple_mesh_.get_vert_amt();

        for (const auto& s_v : simple_vert) 
        {
            float min_dist = Detail::INF;
            for (const auto& o_v : orig_vert) 
            {
                float dist = Vec3::distance(s_v, o_v);
                if (dist < min_dist) 
                    min_dist = dist;
            }
            
            if (min_dist == Detail::INF)
                throw std::runtime_error("count_rms: failed to find nearest vertex");
            
            sum_dist_sq += min_dist * min_dist;
        }

        if (s_v_amt == 0)
            throw std::runtime_error("count_rms: simplified mesh has zero vertices");

        rms_ = std::sqrt(sum_dist_sq / s_v_amt);
        
        if (std::isnan(rms_) || std::isinf(rms_))
            throw std::runtime_error("count_rms: invalid RMS value (NaN or Inf)");

        return rms_;
    }
};

}