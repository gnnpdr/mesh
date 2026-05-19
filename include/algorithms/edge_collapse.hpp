/**
 * @file edge_collapse.hpp
 * @brief Реализация алгоритмов упрощения сетки через схлопывание рёбер
 * 
 * В файле несколько реализаций классов, соответствующим двум вариациям алгоритмма, отличающимся методом выбора ребра для схлопывания и положени результирующей точки.
 * SimpleEdgeCollapse - метрика на основе длины ребра
 * QuadricEdgeCollapse - метрика на основе квадратичных ошибок
 * 
 * @see EdgeCollapse
 * @see SimpleEdgeCollapse
 * @see QuadricEdgeCollapse
 */

#pragma once

#include "mesh/mesh.hpp"
#include <queue>
#include <memory>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <Eigen/Dense>

namespace EdgeCollapse
{
    using Mesh::EdgeInd;
    using Mesh::EdgeMesh;
    using Mesh::VertexInd;
    using Mesh::TriangleInd;
    using Mesh::Triangle;

/**
 * @brief Структура для хранения ребра в куче
 */
struct EdgePtr 
{
    EdgeInd edge_ind_;   ///< Индекс ребра в векторе EdgeMesh
    float cost_;         ///< Стоимость схлопывания ребра

    /**
     * @brief Оператор сравнения для приоритетной очереди (min-heap)
     */
    bool operator<(const EdgePtr& other) const 
    {
        return cost_ > other.cost_; 
    }
};

namespace Detail
{
    static VertexInd ERROR_VAL = -1;        ///< Значение ошибки для невалидного индекса
    static size_t MAX_TRIANGLES_AMT = 4;    ///< Минимальное допустимое количество треугольников
}

/**
 * @brief Абстрактный базовый класс для алгоритмов схлопывания рёбер
 * 
 * Управляет выбором, удалением и порядком ребер
 * 
 * Дочерние обяханы вычислить новую позицию, задать стоимость и выполнить схлопывание
 */
class EdgeCollapse
{
protected:
    EdgeMesh& mesh_;                          ///< Ссылка на упрощаемую сетку
    std::priority_queue<EdgePtr> edge_heap_;  ///< Куча рёбер, отсортированная по стоимости
    float detail_level_ = 0.5f;               ///< Степень детализации
    size_t final_triangles_amt_ = 400;        ///< Целевое количество треугольников

    /**
     * @brief Вычисляет новую позицию для схлопнутой вершины
     */
    virtual Vec3f calc_new_pos(VertexInd v1, VertexInd v2) = 0;

    /**
     * @brief Вычисляет стоимость схлопывания ребра
     */
    virtual float calc_edge_cost(EdgeInd e) = 0;

    /**
     * @brief Строит кучу из всех активных рёбер
     * @throws std::runtime_error Если не удалось построить кучу
     */
    void build_heap();

    /**
     * @brief Удаляет вырожденные треугольники
     */
    void remove_degenerate_triangles();

public:

    explicit EdgeCollapse(Mesh::EdgeMesh& mesh) : mesh_(mesh) {}

    EdgeCollapse(Mesh::EdgeMesh& mesh, size_t final_triangles_amt) : mesh_(mesh), final_triangles_amt_(final_triangles_amt) 
    {
        if (final_triangles_amt < 1)
            throw std::invalid_argument("EdgeCollapse: final_triangles_amt must be at least 1");
    }

    EdgeCollapse(Mesh::EdgeMesh& mesh, float detail_level) : mesh_(mesh), detail_level_(detail_level), final_triangles_amt_(calc_final_triangles_amt()) 
    {
        if (detail_level < 0.0f || detail_level > 1.0f)
            throw std::invalid_argument("EdgeCollapse: detail_level must be in [0, 1], got " + std::to_string(detail_level));
    }

    EdgeCollapse(const EdgeCollapse&) = delete;  
    EdgeCollapse& operator=(const EdgeCollapse&) = delete;
    EdgeCollapse(EdgeCollapse&&) = default;
    EdgeCollapse& operator=(EdgeCollapse&&) = default;

    /**
     * @brief Запускает процесс упрощения сетки
     * @return Ссылка на упрощённую сетку
     * @throws std::runtime_error Если упрощение не удалось или сетка пуста
     */
    EdgeMesh& simplify();

    virtual ~EdgeCollapse() = default;

private:
    /**
     * @brief Вычисляет целевое количество треугольников из степени детализации
     */
    size_t calc_final_triangles_amt()
    {
        size_t orig_triangles_amt = mesh_.get_triang_amt();
        if (orig_triangles_amt == 0)
            throw std::runtime_error("EdgeCollapse: mesh has no triangles");
        
        size_t final_triangles_amt = orig_triangles_amt * detail_level_;

        if (final_triangles_amt < Detail::MAX_TRIANGLES_AMT)
            final_triangles_amt = Detail::MAX_TRIANGLES_AMT;
        if (final_triangles_amt > orig_triangles_amt)
            final_triangles_amt = orig_triangles_amt;

        return final_triangles_amt;
    }

    EdgePtr get_best_edge();

    /**
     * @brief Выполняет схлопывание
     */
    virtual void collapse_edge(EdgeInd edge_ind) = 0;    

    /**
     * @brief Обновляет стоимости затронутых рёбер
     */
    void update_edges_in_heap(EdgeInd collapsed_edge_ind);

    /**
     * @brief Полностью перестраивает кучу из активных рёбер
     * @note Используется, когда куча становится слишком большой
     */
    void rebuild_heap() 
    {
        std::priority_queue<EdgePtr> new_heap;
        for (EdgeInd e = 0; e < mesh_.get_edges().size(); e++) 
        {
            if (mesh_.get_edges()[e].is_active_) 
            {
                float cost = calc_edge_cost(e);
                new_heap.push({e, cost});
            }
        }
        edge_heap_ = std::move(new_heap);
    }
};

//----------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Упрощение сетки на основе длины рёбер
 * 
 * Чем короче ребро, тем меньше его стоимость, и тем вероятнее оно будет схлопнуто
 */
class SimpleEdgeCollapse : public EdgeCollapse
{
protected:
    /**
     * @brief Вычисляет новую позицию вершины как середину ребра
     */
    Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override
    {
        auto& vertices = mesh_.get_vertices();
        if (v1 >= vertices.size() || v2 >= vertices.size())
            throw std::out_of_range("SimpleEdgeCollapse: vertex index out of range");
        return (vertices[v1] + vertices[v2]) * 0.5f;
    }

    /**
     * @brief Вычисляет стоимость ребра как евклидово расстояние
     */
    float calc_edge_cost(EdgeInd e) override
    {
        auto& edges = mesh_.get_edges();
        auto& vertices = mesh_.get_vertices();
        
        if (e >= edges.size())
            throw std::out_of_range("SimpleEdgeCollapse: edge index out of range");

        const auto& v1 = vertices[edges[e].v1_];
        const auto& v2 = vertices[edges[e].v2_];
        
        return Vec3::distance(v1, v2);
    }

    void collapse_edge(EdgeInd edge_ind) override;

public:
    SimpleEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
        build_heap();
    }

    ~SimpleEdgeCollapse() = default;
};

//----------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Упрощение сетки на основе квадратичных метрик ошибки
 * 
 * Наиболее качественный алгоритм упрощения. Использует метод Гарленда-Хекберта
 * Для каждой вершины хранится матрица 4x4, представляющая сумму плоскостей всех инцидентных треугольников. Это позволяет сохранять детали формы
 */
class QuadricEdgeCollapse : public EdgeCollapse
{
    std::vector<Eigen::Matrix4f> vertex_Qs_;  ///< Квадрики для каждой вершины

protected:
    Vec3f calc_new_pos(VertexInd v1, VertexInd v2) override;
    float calc_edge_cost(EdgeInd e) override;

    /**
     * @brief Пересчитывает квадрику для указанной вершины с нуля
     */
    void update_quadrics_around_vertex(VertexInd v) 
    {
        if (v >= vertex_Qs_.size())
            throw std::out_of_range("QuadricEdgeCollapse: vertex index out of range");
        
        vertex_Qs_[v] = Eigen::Matrix4f::Zero();
        
        for (TriangleInd t_idx : mesh_.get_vertex_data()[v].incident_triangles_) 
        {
            const auto& t = mesh_.get_triangles()[t_idx];
            Eigen::Matrix4f K = get_K(t);

            if (!has_nan(K) && !is_degenerate(K))
                vertex_Qs_[v] += K;
        }
    }

    void collapse_edge(EdgeInd edge_ind) override;

public:
    QuadricEdgeCollapse(EdgeMesh& mesh, float detail_level) : EdgeCollapse(mesh, detail_level) 
    {
        if (mesh.get_triang_amt() == 0)
            throw std::runtime_error("QuadricEdgeCollapse: mesh has no triangles");
        
        init_Qs();
        build_heap();
    }

    ~QuadricEdgeCollapse() = default;

private:
    /**
     * @brief Вычисляет ошибку в точке для заданной квадрики
     * @param Q Квадрика (матрица 4x4)
     * @param v Точка в пространстве
     * @return Квадратичная ошибка v^T * Q * v
     */
    float evaluate_error(const Eigen::Matrix4f& Q, const Vec3f& v) 
    {
        Eigen::Vector4f v4(v.x(), v.y(), v.z(), 1.0f);
        return v4.transpose() * Q * v4;
    }

    /**
     * @brief Инициализирует квадрики для всех вершин
     * @throws std::runtime_error Если инициализация не удалась
     */
    void init_Qs();

    /**
     * @brief Создаёт квадрику для треугольника
     * @param t Треугольник (индексы вершин)
     * @return Матрица K = p * p^T, где p - вектор плоскости
     */
    Eigen::Matrix4f get_K(const Triangle& t) const;

    /**
     * @brief Проверяет, содержит ли матрица nan значения
     * @param Q Матрица для проверки
     * @return true если есть nan
     */
    static bool has_nan(const Eigen::Matrix4f& Q) {
        return Q.array().isNaN().any();
    }

    /**
     * @brief Проверяет, является ли матрица вырожденной
     * @param Q Матрица для проверки
     * @return true если определитель близок к нулю
     */
    static bool is_degenerate(const Eigen::Matrix4f& Q) {
        Eigen::Matrix3f A = Q.topLeftCorner<3,3>();
        return std::abs(A.determinant()) < 1e-8f;
    }
};

}