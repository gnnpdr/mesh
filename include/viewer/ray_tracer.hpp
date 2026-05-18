#pragma once

#include "mesh/mesh.hpp"

namespace RayTracer
{

struct Settings 
{
    int width = 800;
    int height = 600;
    Vec3f light_pos = Vec3f(5.0f, 10.0f, 5.0f);
    Vec3f light_color = Vec3f(1.0f, 1.0f, 1.0f);
    Vec3f background_color = Vec3f(0.2f, 0.2f, 0.3f);
    Vec3f camera_position = Vec3f(0.0f, 0.0f, -5.0f);
    Vec3f camera_target = Vec3f(0.0f, 0.0f, 0.0f);
    Vec3f camera_up = Vec3f(0.0f, 1.0f, 0.0f);
    float vertical_fov_deg = 45.0f;
    float light_intensity = 1.0f;
    int samples_per_pixel = 4;
};

struct Ray 
{
    Vec3f origin;
    Vec3f direction;
    Ray(const Vec3f& o, const Vec3f& d) : origin(o), direction(d) {}
    Vec3f pointAt(float t) const { return origin + direction * t; }
};
    
struct HitInfo 
{
    bool hit = false;
    Vec3f point;
    Vec3f normal;
    float distance = 0;
    int objectId = -1;
    int triangleId = -1;
};


class RayTracer 
{
    Settings settings_;
    std::vector<Mesh::Mesh> objects_;
    
public:
    RayTracer() = default;

    RayTracer(const Settings& settings) : settings_(settings) {}
    
    void add_object(const Mesh::Mesh& mesh) 
    {
        objects_.push_back(mesh);
    }

    void render(const std::string& output_filename = "raytraced_output.png");
    
private:

    HitInfo intersect(const Ray& ray) const;

    bool intersect_triangle(const Ray& ray, const Vec3f& v0, const Vec3f& v1, 
                          const Vec3f& v2, float& t, float& u, float& v) const;

    Vec3f compute_color(const HitInfo& hit, const Ray& ray) const;

    bool is_in_shadow(const Vec3f& point, const Vec3f& lightDir) const;
    
    void save2png(const std::string& filename, const std::vector<Vec3f>& framebuffer, int width, int height) const;
};

}