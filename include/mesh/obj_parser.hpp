#pragma once
#include "lib/vec3.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace OBJParser
{

namespace Detail
{
    static char COMMENT_SIGN = '#';
    static std::string VERTICE_SIGN = "v";
    static std::string FACE_SIGN = "f";
    static size_t START_IND = 1;
}

/**
 * @brief Makes Mesh structure by obj file
 * 
 * @param vertices_ - mesh vetices vector
 * @param faces_ - mesh triangles vector
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

    const std::vector<Vertex>& get_vertices() const {return vertices_;}
    const std::vector<Face>& get_faces() const {return faces_;}

    void parse(const std::string& filename) 
    {
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file: " + filename);

        std::string line;
        size_t line_number = 0;
        
        try
        {
            while (std::getline(file, line)) 
            {
                line_number++;
                if (line.empty() || line[0] == Detail::COMMENT_SIGN) continue;

                std::istringstream iss(line);
                std::string type;
                if (!(iss >> type))
                    throw std::runtime_error("Empty line at line " + std::to_string(line_number));

                if (type == Detail::VERTICE_SIGN) 
                {
                    float x, y, z;
                    if (!(iss >> x >> y >> z))
                        throw std::runtime_error("Invalid vertex format at line " + std::to_string(line_number) + ": expected 'v x y z'");

                    Vertex v(x, y, z);
                    vertices_.push_back(v);
                }
                else if (type == Detail::FACE_SIGN) 
                {
                    Face f;
                    std::string vertex_data;
                    int vertex_count = 0;
                
                    while (iss >> vertex_data) 
                    {
                        vertex_count++;
                        int ind = parce_ind(vertex_data, vertices_.size());
                        
                        if (ind < 0 || ind >= static_cast<int>(vertices_.size())) 
                        {
                            throw std::runtime_error("Invalid vertex index " + std::to_string(ind) + 
                                                     " at line " + std::to_string(line_number) + 
                                                     " (vertices range: 0.." + std::to_string(vertices_.size() - 1) + 
                                                     ", got: " + std::to_string(ind) + ")");
                        }
                        
                        if (ind > vertices_.size()) 
                        {
                            throw std::runtime_error("Vertex index out of range: " + std::to_string(ind) + 
                                                     " > " + std::to_string(vertices_.size()));
                        }
                        
                        f.push_back(ind);
                    }
                
                    if (vertex_count < 3)
                        throw std::runtime_error("Face with less than 3 vertices at line " + std::to_string(line_number));
                
                    faces_.push_back(f);
                }
            }
        }
        catch (const std::runtime_error& e) 
        {
            throw std::runtime_error("Error parsing " + filename + ": " + e.what());
        }

        if (vertices_.empty())
            throw std::runtime_error("File " + filename + " contains no vertices");

        if (faces_.empty()) 
            throw std::runtime_error("File " + filename + " contains no faces");

        file.close();
    }

private:

    int parce_ind(const std::string& token, const size_t cur_vert_amt) 
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
        catch (...)
        {
            throw std::runtime_error("Invalid vertex index: " + indexStr);
        }

        if (ind == 0)
            throw std::runtime_error("Vertex index cannot be 0");

        int result;
        if (ind < 0) 
        {
            result = static_cast<int>(cur_vert_amt) + ind;
            if (result < 0)
                throw std::runtime_error("Negative vertex index out of range: " + std::to_string(ind));
        } 
        else 
        {
            result = ind - 1; 
            if (result >= static_cast<int>(cur_vert_amt)) 
            {
                throw std::runtime_error("Vertex index out of range: " + std::to_string(ind) + 
                                         " > " + std::to_string(cur_vert_amt));
            }
        }

        return result;
    }
};
}