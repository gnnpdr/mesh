/**
 * @file obj_converter.hpp
 * @brief Конвертер полигональных сеток в формат OBJ
 * 
 * Позволяет сохранить Mesh в текстовый OBJ файл
 * Формат:
 * v x y z — вершина
 * f i j k — треугольник (индексы начинаются с 1)
 */

#pragma once
#include "mesh/mesh.hpp"
#include <fstream>

namespace MeshOBJConverter
{
/**
 * @brief Класс для сохранения сетки в OBJ файл
 * 
 * @note Индексы треугольников автоматически преобразуются из 0-индексации (внутреннее представление) в 1-индексацию (формат OBJ)
 */
class MeshOBJConverter
{
    Mesh::Mesh mesh_;

public:

    MeshOBJConverter(const Mesh::Mesh& mesh) : mesh_(mesh) 
    {
        if (mesh.is_empty())
            throw std::invalid_argument("MeshOBJConverter: mesh is empty, nothing to save");
    }

    void convert(const std::string& filename = "new_mesh.obj") const
    {
        std::ofstream file(filename);
        if (!file.is_open()) 
             throw std::runtime_error("MeshOBJConverter: failed to open file '" + filename + "'");

        file << "#woooow, your mesh is sooo big\n#but i think i can handle it\n" << std::endl;

        auto& vertices = mesh_.get_vertices();
        file << "#sir, your vertices, sir" << std::endl;
        for (const auto& v : vertices)
            file << "v " << v.x() << " " << v.y() << " " << v.z() << std::endl;
        
        file << std::endl;
        auto& tringles = mesh_.get_triangles();
        file << "#sir, your triangles, sir" << std::endl;
        for (const auto& t : tringles)
            file << "f " << t[0] + 1 << " " <<  t[1] + 1 << " " << t[2] + 1 << std::endl;

        file << std::endl;
        file << "#fooh, it was difficult, sir, im breathless now" << std::endl;

        file.close(); 
    }
};

}