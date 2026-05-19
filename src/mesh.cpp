#include "mesh/mesh.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

Mesh::Mesh::Mesh(const OBJParser::OBJParser& parser)
{
    const auto& parser_vertices = parser.get_vertices();
    const auto& parser_faces = parser.get_faces();

    if (parser_vertices.empty())
        throw std::runtime_error("Mesh: parser contains no vertices");

    if (parser_faces.empty()) 
        throw std::runtime_error("Mesh: parser contains no faces");

    vertices_.reserve(parser_vertices.size());
    for (const auto& v : parser_vertices) 
        vertices_.push_back(v);

    for (const auto& f : parser_faces) 
    {
        if (f.size() < 3)
            throw std::runtime_error("Mesh: face has less than 3 vertices");

        for (size_t i = 1; i + 1 < f.size(); i++)
            add_triangle(f[0], f[i], f[i + 1]);
    }

    bounding_box();
}

void Mesh::Mesh::print() const
{
    std::cout << "=== Mesh Information ===" << std::endl;
    std::cout << "Vertices: " << vertices_.size() << std::endl;
    std::cout << "Triangles: " << triangles_.size() << std::endl;
    
    if (!vertices_.empty()) 
    {
        std::cout << "Bounding box: [" 
                  << bounding_box_[0].x() << ", " << bounding_box_[1].x() << "] x ["
                  << bounding_box_[0].y() << ", " << bounding_box_[1].y() << "] x ["
                  << bounding_box_[0].z() << ", " << bounding_box_[1].z() << "]"
                  << std::endl;
    }

    std::cout << "\nVertices:" << std::endl;
    for (size_t i = 0; i < vertices_.size(); ++i) 
    {
        const auto& v = vertices_[i];
        std::cout << "  v" << i << ": " << v.x() << " " << v.y() << " " << v.z() << std::endl;
    }

    std::cout << "\nTriangles:" << std::endl;
    for (size_t i = 0; i < triangles_.size(); ++i) 
    {
        const auto& t = triangles_[i];
        std::cout << "  f" << i << ": " << t[0] << " " << t[1] << " " << t[2] << std::endl;
    }
}

std::array<float, 3> Mesh::Mesh::get_bounding_box_size() const 
{
    if (vertices_.empty())
        return {0.0f, 0.0f, 0.0f};
    
    const Vec3f& min_bound = bounding_box_[0];
    const Vec3f& max_bound = bounding_box_[1];

    float x = max_bound.x() - min_bound.x();
    float y = max_bound.y() - min_bound.y();
    float z = max_bound.z() - min_bound.z();

    return {x, y, z};
}

float Mesh::Mesh::get_bounding_box_diag_size() const
{
    if (vertices_.empty())
        return 0.0f;
    
    const Vec3f& min_bound = bounding_box_[0];
    const Vec3f& max_bound = bounding_box_[1];

    float x = max_bound.x() - min_bound.x();
    float y = max_bound.y() - min_bound.y();
    float z = max_bound.z() - min_bound.z();

    return std::sqrt(x * x + y * y + z * z);
}

void Mesh::Mesh::bounding_box()
{
    if (vertices_.empty()) 
    {
        bounding_box_[0] = Vec3f(0.0f, 0.0f, 0.0f);
        bounding_box_[1] = Vec3f(0.0f, 0.0f, 0.0f);
        return;
    }

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

//--EdgeMesh-----------------------------------------------------------------------------------------

void Mesh::EdgeMesh::print() const
{
    std::cout << "=== EdgeMesh Information ===" << std::endl;
    std::cout << "Vertices: " << vertices_.size() << std::endl;
    std::cout << "Triangles: " << triangles_.size() << std::endl;
    std::cout << "Edges: " << edges_.size() << std::endl;
    
    int active_vertices = 0;
    int active_edges = 0;
    for (const auto& vd : vertex_data_)
        if (vd.is_active_) active_vertices++;

    for (const auto& e : edges_) 
        if (e.is_active_) active_edges++;
    
    std::cout << "Active vertices: " << active_vertices << std::endl;
    std::cout << "Active edges: " << active_edges << std::endl;
    
    std::cout << "\nVertices:" << std::endl;
    for (size_t i = 0; i < vertices_.size(); ++i) 
    {
        std::cout << "  v" << i << ": " << vertices_[i].x() << " " << vertices_[i].y() << " " << vertices_[i].z() << std::endl;
    }

    std::cout << "\nTriangles:" << std::endl;
    for (size_t i = 0; i < triangles_.size(); ++i) 
    {
        const auto& t = triangles_[i];
        std::cout << "  f" << i << ": " << t[0] << " " << t[1] << " " << t[2] << std::endl;
    }

    std::cout << "\nVertex Data:" << std::endl;
    for (size_t i = 0; i < vertex_data_.size(); ++i) 
    {
        const auto& vd = vertex_data_[i];
        std::cout << "  Vertex " << i << ":" << std::endl;
        std::cout << "    Neighbors: ";
        for (int n : vd.neighbor_vertices_) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "    Incident triangles: ";
        for (int t : vd.incident_triangles_) std::cout << t << " ";
        std::cout << std::endl;
        std::cout << "    Active: " << vd.is_active_ << std::endl;
    }

    std::cout << "\nEdges:" << std::endl;
    for (size_t i = 0; i < edges_.size(); ++i) 
    {
        const auto& e = edges_[i];
        std::cout << "  e" << i << ": v1=" << e.v1_ << " v2=" << e.v2_ << " t1=" << e.t1_ << " t2=" << e.t2_ << " active=" << e.is_active_ << std::endl;
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
            if (t[v] < 0 || t[v] >= v_amt)
                throw std::out_of_range("collect_vertices_data: vertex index out of range");
        }
        
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
    {
        if (edges_[edge->second].t2_ == -1)
            edges_[edge->second].t2_ = t_ind;
    }
}