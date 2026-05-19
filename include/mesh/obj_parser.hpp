/**
 * @file obj_parser.hpp
 * @brief Парсер OBJ файлов
 */

#pragma once

#include "lib/vec3.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace OBJParser
{

namespace Detail
{
    static char COMMENT_SIGN = '#';           ///< Символ начала комментария
    static std::string VERTICE_SIGN = "v";    ///< Маркер вершины
    static std::string FACE_SIGN = "f";       ///< Маркер грани
    static size_t START_IND = 1;              ///< Начальный индекс в OBJ
}

/**
 * @brief Парсер OBJ файлов
 * 
 * Читает OBJ файл и извлекает вершины и треугольные грани
 * Фрматы:
 * - v x y z
 * - f v1 v2 v3
 * - f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 (игнорирует текстурные координаты и нормали)
 * - Отрицательные индексы (относительно конца списка)
 * 
 * @note Все не-треугольные грани преобразуются в треугольники
 * @note Текстурные координаты и нормали игнорируются
 */
class OBJParser 
{
    using Vertex = Vec3f;              
    using VertexInd = size_t;        
    using Face = std::vector<VertexInd>;

    std::vector<Vertex> vertices_;  
    std::vector<Face> faces_;     
    
public:
    OBJParser(const std::string& filename)
    {
        parse(filename);
    }

    const std::vector<Vertex>& get_vertices() const { return vertices_; }
    
    const std::vector<Face>& get_faces() const { return faces_; }
    
    size_t get_vertex_count() const { return vertices_.size(); }
    
    size_t get_face_count() const { return faces_.size(); }
    
    bool is_empty() const { return vertices_.empty() || faces_.empty(); }

    void parse(const std::string& filename) 
    {
        std::ifstream file(filename);
        if (!file.is_open()) 
            throw std::runtime_error("OBJParser: cannot open file: " + filename);

        std::string line;
        size_t line_number = 0;
        
        try
        {
            while (std::getline(file, line)) 
            {
                line_number++;
                
                if (line.empty() || line[0] == Detail::COMMENT_SIGN) 
                    continue;

                std::istringstream iss(line);
                std::string type;
                if (!(iss >> type)) 
                    throw std::runtime_error("Empty line at line " + std::to_string(line_number));

                if (type == Detail::VERTICE_SIGN) 
                {
                    float x, y, z;
                    if (!(iss >> x >> y >> z)) 
                        throw std::runtime_error("Invalid vertex format at line " + std::to_string(line_number) + ": expected 'v x y z'");
                    vertices_.push_back(Vertex(x, y, z));
                }
                else if (type == Detail::FACE_SIGN) 
                {
                    Face f;
                    std::string vertex_data;
                    int vertex_count = 0;
                
                    while (iss >> vertex_data) 
                    {
                        vertex_count++;
                        int ind = parse_index(vertex_data, vertices_.size());
                        
                        f.push_back(ind);
                    }
            
                    if (vertex_count < 3) 
                        throw std::runtime_error("Face with less than 3 vertices at line " + std::to_string(line_number));
                    
                    if (vertex_count > 3) 
                    {
                        for (int i = 2; i < vertex_count; i++) 
                        {
                            Face triangulated_face;
                            triangulated_face.push_back(f[0]);
                            triangulated_face.push_back(f[i-1]);
                            triangulated_face.push_back(f[i]);
                            faces_.push_back(triangulated_face);
                        }
                    } 
                    else
                        faces_.push_back(f);
                }
            }
        }
        catch (const std::runtime_error& e) 
        {
            throw std::runtime_error("Error parsing " + filename + " at line " + std::to_string(line_number) + ": " + e.what());
        }

        if (vertices_.empty())
            throw std::runtime_error("OBJParser: file " + filename + " contains no vertices");

        if (faces_.empty())
            throw std::runtime_error("OBJParser: file " + filename + " contains no faces");

        file.close();
    }

private:
    int parse_index(const std::string& token, const size_t cur_vert_amt) 
    {
        if (token.empty()) 
            throw std::runtime_error("Empty vertex index token");

        size_t slash_pos = token.find('/');
        std::string indexStr = (slash_pos != std::string::npos) ? token.substr(0, slash_pos) : token;

        int ind;
        try 
        {
            ind = std::stoi(indexStr);
        } 
        catch (const std::exception& e)
        {
            throw std::runtime_error("Invalid vertex index: '" + indexStr + "'");
        }

        if (ind == 0) 
            throw std::runtime_error("Vertex index cannot be 0");

        int result;
        if (ind < 0) 
        {
            result = cur_vert_amt + ind;
            if (result < 0)
                throw std::runtime_error("Negative vertex index out of range: " + std::to_string(ind));
        } 
        else 
        {
            result = ind - 1; 
            if (result >= cur_vert_amt) 
                throw std::runtime_error("Vertex index out of range: " + std::to_string(ind) + " > " + std::to_string(cur_vert_amt));
        }

        return result;
    }
};

}