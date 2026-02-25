#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace OBJParser
{

/*using Vertice = Vec3::Vec3f;
    using VerticeInd = size_t;
    using Triangle = std::array<VerticeInd, 3>;

    std::vector<Vertice> vertices_;
    std::vector<Triangle> triangles_;

    Vec3(T x, T y, T z) : x_(x), y_(y), z_(z) {}
*/

/*struct Vertex 
{
    float x_, y_, z_;
};

struct Face 
{
    std::vector<size_t> vertex_indices_;
};

class OBJParser 
{
    std::vector<Vertex> vertices_;
    std::vector<Face> faces_;
    
public:

    const std::vector<Vertex>& 

    bool parse(const std::string& filename) 
    {
        std::ifstream file(filename);
        std::string line;
        
        while (std::getline(file, line)) 
        {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            
            if (type == "v") 
            {
                Vertex v;
                iss >> v.x_ >> v.y_ >> v.z_;
                vertices_.push_back(v);
            }
            else if (type == "f") 
            {
                Face f;
                std::string vertex_data;
                while (iss >> vertex_data) 
                {
                    // Формат: f v1/vt1/vn1 v2/vt2/vn2, пока что мы задаембез нормалей и текстурных координат, вообще формат будет типа v/vt/vn или v//vn или просто v, поэтому он должен просто пройтись без /
                    std::istringstream viss(vertex_data);
                    std::string index_str;
                    //std::getline(viss, index_str, '/');
                    f.vertex_indices_.push_back(std::stoi(index_str));
                }
                faces_.push_back(f);
            }
        }
        return true;
    }*/
};
}