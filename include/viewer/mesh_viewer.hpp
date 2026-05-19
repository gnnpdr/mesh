/**
 * @file mesh_viewer.hpp
 * @brief Для визуализации сеток
 * 
 * Просмотр через Polyscope, возможно несколько объектов в сцене, три алгоритма упрощения ( можнодобавить свой через плагинную систему),рендеринг с тенями, вычисление метрик
 */

#pragma once

#include "algorithms/vertex_cluster.hpp"
#include "algorithms/metrics.hpp"
#include "algorithms/edge_collapse.hpp"
#include "viewer/ray_tracer.hpp"
#include "algorithms/simplifier_registry.hpp"
#include "algorithms/adapters.hpp"

#include <thread>
#include <memory>
#include <string>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <iostream>

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include <polyscope/curve_network.h>
#include "polyscope/camera_parameters.h"
#include <glm/gtc/matrix_transform.hpp>

struct ImVec4;

namespace MeshViewer
{

struct SceneObject 
{
    std::string name;                           ///< Имя объекта
    Mesh::Mesh mesh;                            ///< Текущая сетка (может быть упрощённой)
    Mesh::Mesh orig_mesh;                       ///< Оригинальная сетка (для сброса)
    Vec3f color = Vec3f(0.7f, 0.7f, 0.7f);     ///< Цвет отображения
    Vec3f position = Vec3f(0.0f, 0.0f, 0.0f);   ///< Позиция в пространстве
    bool visible = true;                        ///< Видимость
    float detail_level = 1.0f;                  ///< Текущая степень детализации
    bool is_original = true;                    ///< Является ли исходной моделью
    
    /**
     * @brief Указатель на структуру Polyscope для управления отображением
     */
    polyscope::SurfaceMesh* ps_handle = nullptr;
    
    SceneObject(const std::string& name, const Mesh::Mesh& orig_mesh) : name(name), orig_mesh(orig_mesh), mesh(orig_mesh) {}
    
    SceneObject() = default;
    
    ~SceneObject() 
    {
        if (ps_handle)
            polyscope::removeSurfaceMesh(ps_handle->name);
    }
};


class MeshViewer
{
    Mesh::Mesh orig_mesh_;        ///< Исходная сетка (загруженная из файла)
    Mesh::Mesh current_mesh_;     ///< Текущая упрощённая сетка
    float detail_level_ = 0.0f;   ///< Текущая степень детализации

    std::string algo_name_ = "Vertex cluster";  ///< Имя текущего алгоритма
    float current_hausdorff_ = 0.0f;            ///< Текущее расстояние Хаусдорфа
    float current_rms_ = 0.0f;                  ///< Текущая среднеквадратичная ошибка

    float min_detail_ = 0.0f;    ///< Минимальная степень детализации
    float max_detail_ = 1.0f;    ///< Максимальная степень детализации

    std::vector<std::unique_ptr<SceneObject>> objects_;  ///< Все объекты сцены
    SceneObject* selected_object_ = nullptr;             ///< Выбранный объект

    char new_object_filename_[256] = "";    ///< Имя файла для загрузки
    char new_object_name_[256] = "";        ///< Имя нового объекта
    char new_photo_name_[256] = "";         ///< Имя файла для ray tracing
    Vec3f light_pos_ = Vec3f(5.0f, 10.0f, 5.0f);  ///< Позиция источника света

    std::vector<std::unique_ptr<MeshSimplify::ISimplifier>> simplifiers_;  ///< Зарегистрированные алгоритмы
    int current_simplifier_index_ = 0;      ///< Индекс выбранного алгоритма

    /**
     * @brief Обновляет диапазон значений ползунка детализации
     */
    void update_detail_range();

public:

    MeshViewer(const Mesh::Mesh& orig_mesh) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh) 
    {
        if (orig_mesh.is_empty())
            throw std::invalid_argument("MeshViewer: original mesh is empty");

        update_detail_range();
        update_metrics();
        reload_simplifiers();
    }
    
    MeshViewer(const Mesh::Mesh& orig_mesh, float detail_level) : orig_mesh_(orig_mesh), current_mesh_(orig_mesh), detail_level_(detail_level) 
    {
        update_detail_range();
        update_metrics();
        reload_simplifiers();
    }
    
    MeshViewer() 
    {
        reload_simplifiers(); 
    }
    
    /**
     * @brief Запускает просмотр в Polyscope
     */
    void start_viewer();

    /**
     * @brief Перезагружает список алгоритмов из реестра
     */
    void reload_simplifiers() 
    {
        simplifiers_.clear();
        auto& registry = MeshSimplify::SimplifierRegistry::instance();
        for (const auto& name : registry.get_names()) 
            simplifiers_.push_back(registry.create(name));
    }

private:

    void render_raytraced(const std::string& photo_name);
    void simplify_and_update();
    void detail_level_slider();

    inline void draw_ui() 
    {
        if (simplifiers_.empty()) 
        {
            ImGui::TextColored(ImVec4(1,0,0,1), "No algorithms loaded!");
            return;
        }

        draw_simplification_ui();
        ImGui::Separator();
        draw_scene_ui();
    }

    void draw_simplification_ui();
    void draw_scene_ui();
    void load_file_button();
    void add_button();
    void remove_button();
    void ray_trace_button();
    void tune_properties();
    
    void metrics_callback();
    
    inline void update_metrics() 
    {
        if (!selected_object_) return;

        Metrics::Metrics metrics(selected_object_->orig_mesh, selected_object_->mesh);
        current_hausdorff_ = metrics.get_hausdorff_norm();
        current_rms_ = metrics.get_rms_norm();
    }

    inline ImVec4 get_color_for_metrics(float value, float green_edge = 0.01f, float yellow_edge = 1) 
    {
        if (value < green_edge) return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        if (value < yellow_edge) return ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
        return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    }

    inline void polyscope_object_delete(SceneObject* obj) 
    {
        if (obj->ps_handle) 
        {
            std::string name = obj->ps_handle->name;
            polyscope::removeSurfaceMesh(name);
            obj->ps_handle = nullptr;
        }
    }

    void register_object(SceneObject* obj);
    
    inline void apply_transform(SceneObject* obj) 
    {
        if (!obj->ps_handle) return;
        
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(obj->position.x(), obj->position.y(), obj->position.z()));
        obj->ps_handle->setTransform(transform);
    }

    void load_object_from_file(const std::string& filename, const std::string& name, const Vec3f& position = Vec3f(0.0f, 0.0f, 0.0f), const Vec3f& color = Vec3f(0.7f, 0.7f, 0.7f));
};

}