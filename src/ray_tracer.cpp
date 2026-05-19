#include "lib/stb_image_write.h"
#include "viewer/ray_tracer.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include <Eigen/Geometry>
#include <Eigen/Dense>

RayTracer::HitInfo RayTracer::RayTracer::intersect(const Ray& ray) const 
{
    if (objects_.empty())
        return HitInfo();
    
    HitInfo closest;
    closest.distance = std::numeric_limits<float>::max();
    
    for (size_t obj_id = 0; obj_id < objects_.size(); obj_id++) 
    {
        const auto& mesh = objects_[obj_id];
        if (mesh.is_empty()) continue;
        
        const auto& vertices = mesh.get_vertices();
        const auto& triangles = mesh.get_triangles();
        
        for (size_t t_ind = 0; t_ind < triangles.size(); t_ind++) 
        {
            const auto& tri = triangles[t_ind];
            
            if (tri[0] < 0 || tri[0] >= vertices.size() || tri[1] < 0 || tri[1] >= vertices.size() || tri[2] < 0 || tri[2] >= vertices.size())
                continue;
            
            float t, u, v;
            if (intersect_triangle(ray, vertices[tri[0]], vertices[tri[1]], vertices[tri[2]], t, u, v)) 
            {
                if (t > 0.001f && t < closest.distance) 
                {
                    closest.hit = true;
                    closest.distance = t;
                    closest.point = ray.point_at(t);
                    closest.objectId = obj_id;
                    closest.triangleId = t_ind;
                    
                    Vec3f normal = (vertices[tri[1]] - vertices[tri[0]]).cross(vertices[tri[2]] - vertices[tri[0]]).normalized();
                    closest.normal = normal;
                }
            }
        }
    }
    
    return closest;
}

bool RayTracer::RayTracer::intersect_triangle(const Ray& ray, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, float& t, float& u, float& v) const 
{
    // Алгоритм Möller–Trumbore
    Vec3f edge1 = v1 - v0;
    Vec3f edge2 = v2 - v0;
    
    Vec3f perp = ray.direction.cross(edge2);
    float det = edge1.dot(perp);
    
    // Луч параллелен плоскости треугольника
    if (std::abs(det) < 1e-6f) return false;
    
    float inv_det = 1.0f / det;
    Vec3f t_vec = ray.origin - v0;
    
    // Барицентрическая координата u
    u = inv_det * t_vec.dot(perp);
    if (u < 0.0f || u > 1.0f) return false;
    
    Vec3f q = t_vec.cross(edge1);
    
    // Барицентрическая координата v
    v = inv_det * ray.direction.dot(q);
    if (v < 0.0f || u + v > 1.0f) return false;
    
    // Расстояние до пересечения
    t = inv_det * edge2.dot(q);
    
    return t > 1e-6f;
}

bool RayTracer::RayTracer::is_in_shadow(const Vec3f& point, const Vec3f& lightDir) const 
{
    // Смещение, чтобы избежать самопересечения
    Ray shadow_ray(point + lightDir * 1e-4f, lightDir);
    HitInfo hit = intersect(shadow_ray);
    
    if (!hit.hit) return false;
    
    // Расстояние до источника света
    float light_dist = (settings_.light_pos - point).norm();
    
    return hit.distance < light_dist;
}

Vec3f RayTracer::RayTracer::compute_color(const HitInfo& hit, const Ray& ray) const 
{
    if (!hit.hit)
        return settings_.background_color;
    
    // Базовая составляющая
    Vec3f color = Vec3f(0.1f, 0.1f, 0.1f);
    
    // Направление к источнику света
    Vec3f light_dir = (settings_.light_pos - hit.point).normalized();
    
    // Проверка тени
    if (!is_in_shadow(hit.point, light_dir)) 
    {
        // Diffuse (Ламберт)
        float diff = std::max(0.0f, hit.normal.dot(light_dir));
        color = color + settings_.light_color * diff * settings_.light_intensity;
        
        // Specular (блик)
        Vec3f view_dir = (ray.origin - hit.point).normalized();
        Vec3f reflect_dir = (light_dir * (-2.0f * hit.normal.dot(light_dir)) + hit.normal);
        reflect_dir = reflect_dir.normalized();
        float spec = std::pow(std::max(0.0f, view_dir.dot(reflect_dir)), 32.0f);
        color = color + settings_.light_color * spec * 0.5f;
    }
    
    // Ограничиваем значения в диапазоне [0, 1]
    Vec3f color_limited(std::min(1.0f, color.x()), 
                        std::min(1.0f, color.y()), 
                        std::min(1.0f, color.z()));
    
    return color_limited;
}

void RayTracer::RayTracer::render(const std::string& output_filename) 
{
    if (objects_.empty())
        throw std::runtime_error("RayTracer: no objects to render");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto group_bounding_box = Mesh::compute_bounding_box_of_group(objects_, [](const auto& mesh) -> const Mesh::Mesh& { return mesh; });
    
    float scene_size = std::max({group_bounding_box[1].x() - group_bounding_box[0].x(),
                                 group_bounding_box[1].y() - group_bounding_box[0].y(), 
                                 group_bounding_box[1].z() - group_bounding_box[0].z()});
    
    std::vector<Vec3f> framebuffer(settings_.width * settings_.height);
    
    // Параметры камеры
    const Vec3f& origin = settings_.camera_position;
    const Vec3f& target = settings_.camera_target;
    const Vec3f up = settings_.camera_up;
    float fov_rad = settings_.vertical_fov_deg * 3.14159f / 180.0f;
    float aspect = settings_.width / settings_.height;
    
    // Вычисление базиса камеры
    Vec3f forward = (target - origin).normalized();
    Vec3f right = forward.cross(up).normalized();
    Vec3f real_up = right.cross(forward);
    
    // Настройки суперсэмплинга
    int sqrt_samples = std::sqrtf(settings_.samples_per_pixel);
    if (sqrt_samples < 1) sqrt_samples = 1;
    int total_samples = sqrt_samples * sqrt_samples;
    
    // Рендеринг (параллельный с OpenMP)
    #pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < static_cast<int>(settings_.height); y++) 
    {
        // Генератор случайных чисел для каждого потока
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int x = 0; x < settings_.width; x++) 
        {
            Vec3f color(0.0f, 0.0f, 0.0f);
            
            // Суперсэмплинг (SSAA)
            for (int sy = 0; sy < sqrt_samples; sy++) 
            {
                for (int sx = 0; sx < sqrt_samples; sx++) 
                {
                    // Случайное смещение внутри пикселя
                    float offset_x = dist(rng);
                    float offset_y = dist(rng);
                    
                    // Нормализованные координаты экрана
                    float u = (2.0f * (x + (sx + offset_x) / sqrt_samples) / settings_.width - 1.0f) 
                              * std::tan(fov_rad / 2.0f) * aspect;
                    float v = (1.0f - 2.0f * (y + (sy + offset_y) / sqrt_samples) / settings_.height) 
                              * std::tan(fov_rad / 2.0f);
                    
                    Vec3f direction = (forward + right * u + real_up * v).normalized();
                    Ray ray(origin, direction);
                    
                    HitInfo hit = intersect(ray);
                    color = color + compute_color(hit, ray);
                }
            }
            
            framebuffer[y * settings_.width + x] = color / total_samples;
        }
        
        // Вывод прогресса
        #pragma omp critical
        if (y % 100 == 0) 
        {
            int percent = y / settings_.height * 100.0f;
            std::cout << "Progress: " << percent << "% (" << y << "/" << settings_.height << ")" << std::endl;
        }
    }
    
    save2png(output_filename, framebuffer, settings_.width, settings_.height);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    std::cout << "Saved to: " << output_filename << std::endl;
}

void RayTracer::RayTracer::save2png(const std::string& filename, const std::vector<Vec3f>& framebuffer, int width, int height) const 
{
    if (framebuffer.empty())
        throw std::runtime_error("RayTracer: framebuffer is empty");
    
    if (width <= 0 || height <= 0)
        throw std::runtime_error("RayTracer: invalid image dimensions");
    
    if (framebuffer.size() != width * height)
        throw std::runtime_error("RayTracer: framebuffer size does not match dimensions");
    
    std::vector<unsigned char> pixels(width * height * 3);
    
    for (int i = 0; i < width * height; i++) 
    {
        pixels[i * 3 + 0] = std::clamp(framebuffer[i].x() * 255.0f, 0.0f, 255.0f);
        pixels[i * 3 + 1] = std::clamp(framebuffer[i].y() * 255.0f, 0.0f, 255.0f);
        pixels[i * 3 + 2] = std::clamp(framebuffer[i].z() * 255.0f, 0.0f, 255.0f);
    }
    
    int result = stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3);
    if (result == 0)
        throw std::runtime_error("RayTracer: failed to write PNG file: " + filename);
}