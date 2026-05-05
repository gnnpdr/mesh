#include "lib/stb_image_write.h"

#include "viewer/ray_tracer.hpp"
#include <algorithm>

RayTracer::HitInfo RayTracer::RayTracer::intersect(const Ray& ray) const 
{
    HitInfo closest;
    closest.distance = 1e30f;
    
    for (size_t objId = 0; objId < objects_.size(); objId++) 
    {
        const auto& mesh = objects_[objId];
        const auto& vertices = mesh.get_vertices();
        const auto& triangles = mesh.get_triangles();
        
        for (size_t t_ind = 0; t_ind < triangles.size(); t_ind++) 
        {
            const auto& tri = triangles[t_ind];
            
            float t, u, v;
            if (intersect_triangle(ray, vertices[tri[0]], vertices[tri[1]], vertices[tri[2]], t, u, v)) 
            {
                if (t > 0.001f && t < closest.distance) 
                {
                    closest.hit = true;
                    closest.distance = t;
                    closest.point = ray.pointAt(t);
                    closest.objectId = objId;
                    closest.triangleId = t_ind;
                    
                    // Вычисляем нормаль (интерполированная или просто нормаль треугольника)
                    Vec3::Vec3f normal = (vertices[tri[1]] - vertices[tri[0]]).cross(
                                         vertices[tri[2]] - vertices[tri[0]]).normalize();
                    closest.normal = normal;
                }
            }
        }
    }
    
    return closest;
}

bool RayTracer::RayTracer::intersect_triangle(const Ray& ray, const Vec3::Vec3f& v0, const Vec3::Vec3f& v1, const Vec3::Vec3f& v2, float& t, float& u, float& v) const 
{
    Vec3::Vec3f edge1 = v1 - v0;
    Vec3::Vec3f edge2 = v2 - v0;
    
    Vec3::Vec3f perp = ray.direction.cross(edge2);
    float ray_norm_projection = edge1.dot(perp);
    
    if (std::abs(ray_norm_projection) < 1e-6f) return false;
    
    float inv_det = 1.0f / ray_norm_projection;
    Vec3::Vec3f t_place = ray.origin - v0;
    u = inv_det * t_place.dot(perp);
    
    if (u < 0.0f || u > 1.0f) return false;
    
    Vec3::Vec3f additional = t_place.cross(edge1);
    v = inv_det * ray.direction.dot(additional);
    
    if (v < 0.0f || u + v > 1.0f) return false;
    
    t = inv_det * edge2.dot(additional);
    return t > 1e-6f;
}

bool RayTracer::RayTracer::is_in_shadow(const Vec3::Vec3f& point, const Vec3::Vec3f& lightDir) const 
{
    Ray shadow_ray(point + lightDir * 0.001f, lightDir);
    HitInfo hit = intersect(shadow_ray);
    return hit.hit;
}

Vec3::Vec3f RayTracer::RayTracer::compute_color(const HitInfo& hit, const Ray& ray) const 
{
    if (!hit.hit)
        return settings_.background_color;
    
    Vec3::Vec3f color = Vec3::Vec3f(0.1f, 0.1f, 0.1f);
    
    Vec3::Vec3f light_dir = (settings_.light_pos - hit.point).normalize();
    
    if (!is_in_shadow(hit.point, light_dir)) 
    {
        float diff = std::max(0.0f, hit.normal.dot(light_dir));
        color = color + settings_.light_color * diff * settings_.light_intensity;
        
        Vec3::Vec3f view_dir = (ray.origin - hit.point).normalize();
        Vec3::Vec3f reflect_dir = (light_dir * (-2.0f * hit.normal.dot(light_dir)) + hit.normal);
        reflect_dir = reflect_dir.normalize();
        float spec = std::pow(std::max(0.0f, view_dir.dot(reflect_dir)), 32);
        color = color + settings_.light_color * spec * 0.5f;
    }
    
    Vec3::Vec3f color_limited(std::min(1.0f, color.x()), std::min(1.0f, color.y()), std::min(1.0f, color.z()));
    
    return color_limited;
}

void RayTracer::RayTracer::render(const std::string& output_filename) 
{
    auto group_bounding_box = Mesh::compute_bounding_box_of_group(objects_, [](const auto& mesh) -> const Mesh::Mesh& { return mesh; });
    float size = std::max({group_bounding_box[1].x() - group_bounding_box[0].x(),
                           group_bounding_box[1].y() - group_bounding_box[0].y(), 
                           group_bounding_box[1].z() - group_bounding_box[0].z()});
    
    std::vector<Vec3::Vec3f> framebuffer(settings_.width * settings_.height);
    
    Vec3::Vec3f camera_pos(0.0f, 0.0f, -5.0f);
    Vec3::Vec3f camera_target(0.0f, 0.0f, 0.0f);
    float fov = 45.0f;
    float aspect = (float)settings_.width / settings_.height;
    float tan_half_fov = tanf(fov * 3.14159f / 360.0f);
    
    const Vec3::Vec3f& origin = settings_.camera_position;
    const Vec3::Vec3f& target = settings_.camera_target;
    const Vec3::Vec3f up = settings_.camera_up;
    float fov_rad = settings_.vertical_fov_deg * 3.14159f / 180.0f;

    Vec3::Vec3f forward = (target - origin).normalize();
    Vec3::Vec3f right = forward.cross(up).normalize();
    Vec3::Vec3f real_up = right.cross(forward);
    
    #pragma omp parallel for
    for (int y = 0; y < settings_.height; y++) 
    {
        for (int x = 0; x < settings_.width; x++) 
        {
            float u = (2.0f * (x + 0.5f) / settings_.width - 1.0f) * tan(fov_rad / 2.0f) * aspect;
            float v = (1.0f - 2.0f * (y + 0.5f) / settings_.height) * tan(fov_rad / 2.0f);
            
            Vec3::Vec3f direction = (forward + right * u + real_up * v).normalize();
            direction = direction.normalize();
            Ray ray(origin, direction);

            HitInfo hit = intersect(ray);
            framebuffer[y * settings_.width + x] = compute_color(hit, ray);
        }
        
        if (y % 100 == 0)
            std::cout << "Progress: " << (y * 100 / settings_.height) << "%" << std::endl;
    }
    
    save2png(output_filename, framebuffer, settings_.width, settings_.height);
    std::cout << "Ray tracing completed! Saved to " << output_filename << std::endl;
}

void RayTracer::RayTracer::save2png(const std::string& filename, const std::vector<Vec3::Vec3f>& framebuffer,int width, int height) const 
{
    std::vector<unsigned char> pixels(width * height * 3);
    
    for (int i = 0; i < width * height; i++) 
    {
        pixels[i * 3 + 0] = (unsigned char)(framebuffer[i].x() * 255);
        pixels[i * 3 + 1] = (unsigned char)(framebuffer[i].y() * 255);
        pixels[i * 3 + 2] = (unsigned char)(framebuffer[i].z() * 255);
    }
    
    stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3);
}