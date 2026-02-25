#pragma once
#include "vec3.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace OBJParser
{

namespace Detail
{
    char COMMENT_SIGN = '#';
    std::string VERTICE_SIGN = "v";
    std::string FACE_SIGN = "f";

    size_t START_IND = 1;
}

class OBJParser 
{
    using Vertice = Vec3::Vec3f;
    using VerticeInd = size_t;
    using Face = std::vector<VerticeInd>;

    std::vector<Vertice> vertices_;
    std::vector<Face> faces_;
    
public:

    OBJParser(const std::string& filename)
    {
        parse(filename);
    }

    const std::vector<Vertice>& get_vertices() const {return vertices_;}
    const std::vector<Face>& get_faces() const {return faces_;}

    void parse(const std::string& filename) 
    {
        std::ifstream file(filename);
        std::string line;
        
        while (std::getline(file, line)) 
        {
            if (line.empty() || line[0] == Detail::COMMENT_SIGN) continue;
            
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            
            if (type == Detail::VERTICE_SIGN) 
            {
                float x, y, z;
                iss >> x >> y >> z;
                Vertice v(x, y, z);
                vertices_.push_back(v);
            }
            else if (type == Detail::FACE_SIGN) 
            {
                Face f;
                std::string vertex_data;

                while (iss >> vertex_data) 
                {
                    int ind = parce_ind(vertex_data, vertices_.size());
                    f.push_back(ind);
                }

                faces_.push_back(f);
            }
        }
    }

private:
      
    size_t parce_ind(const std::string& token, const size_t cur_vert_amt) 
    {
        size_t slash_pos = token.find('/');

        std::string indexStr = (slash_pos != std::string::npos) ? token.substr(0, slash_pos) : token;
        
        int ind = std::stoi(indexStr);
        
        if (ind < 0) 
            return cur_vert_amt + ind;
        else 
            return ind - Detail::START_IND;
    }
};
}