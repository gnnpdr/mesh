#include "vertex_cluster.hpp"
#include "edge_collapse.hpp"
#include "mesh_obj_converter.hpp"
#include "mesh_viewer.hpp"
#include "metrics.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    std::string filename = argv[1];
    //printf("%s\n", argv[1]);
    OBJParser::OBJParser parser(filename);
    Mesh::EdgeMesh mesh(parser);

    //mesh.print();

    MeshViewer::MeshViewer viewer(mesh);
    viewer.show_mesh();

    //float detail_level = std::stof(argv[2]);
    //EdgeCollapse::EdgeCollapse claster(mesh, detail_level);
    //Mesh::EdgeMesh new_mesh = claster.simplify();
    //VertexCluster::VertexCluster claster(mesh);
    //Mesh::Mesh new_mesh = claster.simplify();

    //MeshViewer::MeshViewer viewer(mesh);
    //viewer.show_mesh();

    // Metrics::Metrics metrics(mesh, new_mesh);
    // metrics.print();

    //new_mesh.print();

    // MeshOBJConverter::MeshOBJConverter converter(new_mesh);
    // converter.convert();

    //Mesh::EdgeMesh mesh1(mesh);
//
    //mesh1.print();
//
    //Mesh::EdgeMesh mesh2(parser);
//
    //mesh2.print();
//
    //Mesh::EdgeMesh mesh3(mesh.get_vertices(), mesh.get_triangles());
//
    //mesh3.print();

    return 0;
}