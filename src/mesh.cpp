#include "mesh/mesh.hpp"

using Vertex = Vec3f;
using VertexInd = int;
using TriangleInd = int;
using Triangle = std::array<VertexInd, 3>;
using EdgeInd = int;   

Mesh::Mesh::Mesh(const OBJParser::OBJParser& parser)
{
    auto& parser_vertices = parser.get_vertices();
    auto& parser_faces = parser.get_faces();

    for (const auto& v : parser_vertices)
    {
        vertices_.push_back(v);
    }

    for (const auto& f : parser_faces)
    {
        for (size_t i = 1; i + 1 < f.size(); i++) 
        {
           add_triangle(f[0], f[i], f[i + 1]);
        }
    }

    bounding_box();
}

void Mesh::Mesh::print() const
{
    std::cout << "vertices" << std::endl;
    for (auto v : vertices_)
    {
        std::cout << v.x() << " " << v.y() << " " << v.z() << std::endl; 
    }

    std::cout << "triangles" << std::endl;

    for (auto& t : triangles_)
    {
        std::cout << t[0] << " " << t[1] << " " << t[2] << std::endl; 
    }
}

std::array<float, 3> Mesh::Mesh::get_bounding_box_size() const 
{
    Vec3f min_bound = bounding_box_[0];
    Vec3f max_bound = bounding_box_[1];

    float x = max_bound.x() - min_bound.x();
    float y = max_bound.y() - min_bound.y();
    float z = max_bound.z() - min_bound.z();

    return {x, y, z};
}

float Mesh::Mesh::get_bounding_box_diag_size() const
{
    Vec3f min_bound = bounding_box_[0];
    Vec3f max_bound = bounding_box_[1];

    size_t x = max_bound.x() - min_bound.x();
    size_t y = max_bound.y() - min_bound.y();
    size_t z = max_bound.z() - min_bound.z();

    return sqrt(x * x + y * y + z * z);
}

void Mesh::Mesh::bounding_box()
{
    if (vertices_.empty()) return;

    float min_x = vertices_[0].x();
    float min_y = vertices_[0].y();
    float min_z = vertices_[0].z();

    float max_x = vertices_[0].x();
    float max_y = vertices_[0].y();
    float max_z = vertices_[0].z();

    for (const auto& v : vertices_) 
    {
        min_x = std::min(min_x, v.x());
        min_y = std::min(min_y, v.y());
        min_z = std::min(min_z, v.z());

        max_x = std::max(max_x, v.x());
        max_y = std::max(max_y, v.y());
        max_z = std::max(max_z, v.z());
    }

    bounding_box_[0] = Vec3f(min_x, min_y, min_z);
    bounding_box_[1] = Vec3f(max_x, max_y, max_z);
}

 
void Mesh::EdgeMesh::print() const
{
    std::cout << "vertices" << std::endl;
    for (auto v : vertices_)
    {
        std::cout << v.x() << " " << v.y() << " " << v.z() << std::endl; 
    }

    std::cout << "triangles" << std::endl;

    for (const auto& t : triangles_)
    {
        std::cout << t[0] << " " << t[1] << " " << t[2] << std::endl; 
    }

    std::cout << "vertices data" << std::endl;

    for (auto& vd : vertex_data_)
    {
        std::cout << "neighbor vertices" << std::endl;
        const auto& nv = vd.neighbor_vertices_;
        for (const auto v : nv)
        {
            std::cout << v << " ";
        }
        std::cout << std::endl;

        std::cout << "incident triangles" << std::endl;
        const auto& it = vd.incident_triangles_;
        for (const auto t : it)
        {
            std::cout << t << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "edges" << std::endl;

    for (const auto& e : edges_)
    {
        std::cout << "v1 " << e.v1_ << " v2 " << e.v2_ << " t1 " << e.t1_ << " t2 " << e.t2_ <<  std::endl;
    }
}

void Mesh::EdgeMesh::collect_vertices_data(const std::vector<Vertex>& base_vertices, const std::vector<std::array<VertexInd, 3>>& base_triangles)
{
    size_t v_amt = base_vertices.size();
    vertex_data_.resize(v_amt);

    size_t t_amt = base_triangles.size();
    for (size_t t_ind = 0; t_ind < t_amt; t_ind++)
    {
        const auto& t = base_triangles[t_ind];
        for (size_t v = 0; v < 3; v++)
        {
            size_t v_ind = t[v];
            vertex_data_[v_ind].incident_triangles_.push_back(t_ind);

            size_t next_v = t[(v + 1) % 3];
            size_t prev_v = t[(v + 2) % 3];
            vertex_data_[v_ind].neighbor_vertices_.insert(next_v);
            vertex_data_[v_ind].neighbor_vertices_.insert(prev_v);
        }
    }
}

void Mesh::EdgeMesh::collect_edges(const std::vector<std::array<VertexInd, 3>>& base_triangles)
{
    size_t t_amt = base_triangles.size();
    std::map<std::pair<VertexInd, VertexInd>, EdgeInd> edge_map;
    for (size_t t_ind = 0; t_ind < t_amt; t_ind++)
    {
        const auto& t = base_triangles[t_ind];
        add_edge(t[0], t[1], t_ind, edge_map);
        add_edge(t[1], t[2], t_ind, edge_map);
        add_edge(t[2], t[0], t_ind, edge_map);
    }
}

void Mesh::EdgeMesh::add_edge(VertexInd v1, VertexInd v2, TriangleInd t_ind, std::map<std::pair<VertexInd, VertexInd>, EdgeInd>& edge_map)
{
    if (v1 > v2)
        std::swap(v1, v2);

    auto edge = edge_map.find({v1, v2});
    if (edge == edge_map.end())
    {
        Edge new_edge(v1, v2, t_ind);
        edges_.push_back(new_edge);
        edge_map[{v1, v2}] = edges_.size() - 1;    
    }
    else
        edges_[edge->second].t2_ = t_ind;
}