/**
 * @file ray_tracer.hpp
 * @brief Трассировщик лучей для рендеринга сцен с тенями
 * 
 * Модель освещения Phong (ambient + diffuse + specular), тени (shadow rays), суперсэмплинг (anti-aliasing через SSAA), параллелизация через OpenMP
 * 
 */

#pragma once

#include "mesh/mesh.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <lib/stb_image_write.h>

namespace RayTracer
{

/**
 * @brief Настройки рендеринга трассировщика:
 * 
 * Разрешение изображения
 * Положение источника света и его интенсивность
 * Параметры камеры (позиция, направление, FOV)
 * Качество (сэмплы на пиксель для anti-aliasing)
 */
struct Settings 
{
    float width = 800;                          ///< Ширина изображения в пикселях
    float height = 600;                         ///< Высота изображения в пикселях
    Vec3f light_pos = Vec3f(5.0f, 10.0f, 5.0f);  ///< Позиция источника света
    Vec3f light_color = Vec3f(1.0f, 1.0f, 1.0f); ///< Цвет источника света
    Vec3f background_color = Vec3f(0.2f, 0.2f, 0.3f); ///< Цвет фона
    Vec3f camera_position = Vec3f(0.0f, 0.0f, -5.0f); ///< Позиция камеры
    Vec3f camera_target = Vec3f(0.0f, 0.0f, 0.0f);    ///< Точка, на которую смотрит камера
    Vec3f camera_up = Vec3f(0.0f, 1.0f, 0.0f);        ///< Вектор "вверх" для камеры
    float vertical_fov_deg = 45.0f;           ///< Вертикальное поле зрения в градусах
    float light_intensity = 1.0f;             ///< Интенсивность света
    int samples_per_pixel = 4;                ///< Количество сэмплов на пиксель (SSAA)
};

struct Ray 
{
    Vec3f origin;       ///< Начало луча
    Vec3f direction;    ///< Направление луча (нормализовано)
    
    Ray(const Vec3f& o, const Vec3f& d) : origin(o), direction(d) {}
    
    /**
     * @brief Вычисляет точку на луче на расстоянии t
     */
    Vec3f point_at(float t) const { return origin + direction * t; }
};
    
/**
 * @brief Информация о пересечении луча с геометрией
 */
struct HitInfo 
{
    bool hit = false;        ///< Было ли пересечение
    Vec3f point;             ///< Точка пересечения
    Vec3f normal;            ///< Нормаль в точке пересечения
    float distance = 0.0f;   ///< Расстояние от начала луча
    int objectId = -1;       ///< Индекс объекта в сцене
    int triangleId = -1;     ///< Индекс треугольника в объекте
};

/**
 * @brief Класс трассировщика
 */
class RayTracer 
{
    Settings settings_;                 ///< Настройки рендеринга
    std::vector<Mesh::Mesh> objects_;   ///< Список объектов в сцене
    
public:

    RayTracer() = default;

    explicit RayTracer(const Settings& settings) : settings_(settings) {}
    
    void add_object(const Mesh::Mesh& mesh) 
    {
        if (mesh.is_empty())
            throw std::invalid_argument("RayTracer: cannot add empty mesh");
    
        objects_.push_back(mesh);
    }
    
    size_t get_object_count() const { return objects_.size(); }
    
    void clear() { objects_.clear(); }
    
    /**
     * @brief Запускает рендеринг и сохраняет результат в PNG
     * @param output_filename Имя выходного файла (по умолчанию "raytraced_output.png")
     * @throws std::runtime_error Если не удалось сохранить изображение
     */
    void render(const std::string& output_filename = "raytraced_output.png");
    
private:
    /**
     * @brief Находит ближайшее пересечение луча со сценой
     * @param ray Луч для проверки
     * @return Информация о пересечении
     */
    HitInfo intersect(const Ray& ray) const;

    /**
     * @brief Проверяет пересечение луча с треугольником
     */
    bool intersect_triangle(const Ray& ray, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, float& t, float& u, float& v) const;

    /**
     * @brief Вычисляет цвет в точке пересечения
     */
    Vec3f compute_color(const HitInfo& hit, const Ray& ray) const;

    /**
     * @brief Проверяет, находится ли точка в тени
     */
    bool is_in_shadow(const Vec3f& point, const Vec3f& lightDir) const;
    
    /**
     * @brief Сохраняет кадровый буфер в PNG файл
     */
    void save2png(const std::string& filename, const std::vector<Vec3f>& framebuffer, int width, int height) const;
};

}